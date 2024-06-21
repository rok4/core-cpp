/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file Grid.cpp
 ** \~french
 * \brief Implémentation de la classe Grid
 ** \~english
 * \brief Implement class Grid
 */

#include <pthread.h>

#include "processors/Grid.h"
#include <boost/log/trivial.hpp>

#include "utils/Cache.h"

#include <algorithm>
#include <cmath>


Grid::Grid ( int width, int height, BoundingBox<double> bbox ) : width ( width ), height ( height ), bbox ( bbox ) {

    if (width == 0 || height == 0) {
        BOOST_LOG_TRIVIAL(error) <<  "One grid's dimension is null" ;
    }

    nbxReg = 1 + ( width-1 ) /stepInt;
    nbyReg = 1 + ( height-1 ) /stepInt;

    nbx = nbxReg;
    nby = nbyReg;

    /* On veut toujours que le dernier pixel reprojeté soit le dernier de la ligne, ou de la colonne.
     * On ajoute donc toujours le dernier pixel à ceux de la grille, même si celui ci y était déjà.
     * On en ajoute donc un, qui aura potentiellement un écart avec l'avant dernier plus petit (voir même 0).
     * Il faudra donc faire attention à cette différence lors de l'interpolation d'une ligne
     */
    nbx = nbxReg + 1;
    nby = nbyReg + 1;

    endX = width - 1 - ( nbxReg-1 ) * stepInt;
    endY = height - 1 - ( nbyReg-1 ) * stepInt;

    double resX = ( bbox.xmax - bbox.xmin ) / double ( width );
    double resY = ( bbox.ymax - bbox.ymin ) / double ( height );

    // Coordonnées du centre du pixel en haut à droite
    double left = bbox.xmin + 0.5 * resX;
    double top = bbox.ymax - 0.5 * resY;

    // Calcul du pas en unité terrain (et non pixel)
    double stepX = stepInt * resX;
    double stepY = stepInt * resY;

    gridCoords = new PJ_COORD[ nbx * nby ];
    double xtmp, ytmp;

    for ( int y = 0 ; y < nby; y++ ) {
        for ( int x = 0 ; x < nbx; x++ ) {

            if ( y == nbyReg ) {
                // Last reprojected pixel = last pixel
                ytmp = top  - ( height-1 ) *resY;
            } else {
                ytmp = top  - y*stepY;
            }
            if ( x == nbxReg ) {
                // Last reprojected pixel = last pixel
                xtmp = left + ( width-1 ) *resX;
            } else {
                xtmp = left + x*stepX;
            }

            gridCoords[nbx*y + x] = proj_coord(xtmp, ytmp, 0, 0);
        }
    }

    calculateDeltaY();
}

inline void Grid::calculateDeltaY() {
    double min = gridCoords[0].xy.y;
    double max = gridCoords[0].xy.y;
    for ( int x = 1 ; x < nbx; x++ ) {
        min = std::min ( min, gridCoords[x].xy.y );
        max = std::max ( max, gridCoords[x].xy.y );
    }

    deltaY = max - min;
}

void Grid::affine_transform ( double Ax, double Bx, double Ay, double By ) {
    for ( int i = 0; i < nbx*nby; i++ ) {
        gridCoords[i] = proj_coord(Ax * gridCoords[i].xy.x + Bx, Ay * gridCoords[i].xy.y + By, 0, 0);
    }

    // Mise à jour de la bbox
    if ( Ax > 0 ) {
        bbox.xmin = bbox.xmin*Ax + Bx;
        bbox.xmax = bbox.xmax*Ax + Bx;
    } else {
        double xmintmp = bbox.xmin;
        bbox.xmin = bbox.xmax*Ax + Bx;
        bbox.xmax = xmintmp*Ax + Bx;
    }

    if ( Ay > 0 ) {
        bbox.ymin = bbox.ymin*Ay + By;
        bbox.ymax = bbox.ymax*Ay + By;
    } else {
        double ymintmp = bbox.ymin;
        bbox.ymin = bbox.ymax*Ay + By;
        bbox.ymax = ymintmp*Ay + By;
    }

    deltaY = deltaY * fabs ( Ay );
    BOOST_LOG_TRIVIAL(debug) <<   "New first line Y-delta :" << deltaY  ;
}

double Grid::getRatioY()
{
    double ratio = 0;

    if (height == 1) {
        /* Dans le cas d'une hauteur nulle, on ne peut pas faire la différence entre le premier et le dernier pixel de la colonne.
         * On va donc utiliser la bbox. On doit cependant retrancher à la différence ymax-ymin les écarts dûs à la déformation engendrée par la reprojection
         */
        double min = gridCoords[0].xy.y;
        double max = gridCoords[0].xy.y;
        for ( int x = 1 ; x < nbx; x++ ) {
            min = std::min(min, gridCoords[x].xy.y);
            max = std::max(max, gridCoords[x].xy.y);
        }

        double delta = max - min;
        
        return (bbox.ymax - bbox.ymin - delta);
    }

    for ( int x = 0 ; x < nbx; x++ ) {
        ratio = std::max (ratio, fabs( gridCoords[x].xy.y - gridCoords[nbx*(nby-1) + x].xy.y ) / (double) (height - 1));
    }

    return ratio;
}

double Grid::getRatioX()
{
    double ratio = 0;

    if (width == 1) {
        /* Dans le cas d'une largeur nulle, on ne peut pas faire la différence entre le premier et le dernier pixel de la ligne.
         * On va donc utiliser la bbox. On doit cependant retrancher à la différence xmax-xmin les écarts dûs à la déformation engendrée par la reprojection
         */
        double min = gridCoords[0].xy.x;
        double max = gridCoords[0].xy.x;
        for ( int y = 1 ; y < nby; y++ ) {
            min = std::min(min, gridCoords[y*nbx].xy.x);
            max = std::max(max, gridCoords[y*nbx].xy.x);
        }

        double delta = max - min;

        return (bbox.xmax - bbox.xmin - delta);
    }
    
    for ( int y = 0 ; y < nby; y++ ) {
        ratio = std::max (ratio, fabs( gridCoords[nbx*y].xy.x - gridCoords[nbx*(y+1) - 1].xy.x ) / (double) (width - 1));
    }

    return ratio;
}


bool Grid::reproject ( CRS* from_crs, CRS* to_crs ) {
    BOOST_LOG_TRIVIAL(debug) <<   "Grid reprojection: " << from_crs->getRequestCode() <<" -> " << to_crs->getRequestCode()  ;

    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();

    PJ *pj_conv_raw, *pj_conv_normalize;
    pj_conv_raw = proj_create_crs_to_crs_from_pj ( pj_ctx, from_crs->getProjObject(), to_crs->getProjObject(), NULL, NULL);
    if (0 == pj_conv_raw) {
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ pour la reprojection de la grille (création) " << from_crs << " -> " << to_crs << " : " << proj_errno_string ( err )  ;
        return false;
    }

    pj_conv_normalize = proj_normalize_for_visualization(pj_ctx, pj_conv_raw);
    proj_destroy (pj_conv_raw);
    if (0 == pj_conv_normalize) {
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "erreur d initialisation " << from_crs << " " << proj_errno_string ( err )  ;
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ pour la reprojection de la grille (normalisation) " << from_crs << " -> " << to_crs << " : " << proj_errno_string ( err )  ;
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) <<   "Avant (centre du pixel en haut à gauche) "<< gridCoords[0].xy.x << " " << gridCoords[0].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Avant (centre du pixel en haut à droite) "<< gridCoords[nbx-1].xy.x << " " << gridCoords[nbx-1].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Avant (centre du pixel en bas à gauche) "<< gridCoords[nbx*(nby-1)].xy.x << " " << gridCoords[nbx*(nby-1)].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Avant (centre du pixel en bas à droite) "<< gridCoords[nbx*nby-1].xy.x << " " << gridCoords[nbx*nby-1].xy.y  ;

    // On reprojette toutes les coordonnées

    int code = proj_trans_array ( pj_conv_normalize, PJ_FWD, nbx*nby, gridCoords );

    if ( code != 0 ) {
        BOOST_LOG_TRIVIAL(error) <<   "Code erreur proj : " << proj_errno_string(code)  ;
        proj_destroy (pj_conv_normalize);
        return false;
    }

    // On vérifie que le résultat renvoyé par la reprojection est valide
    for ( int i = 0; i < nbx*nby; i++ ) {
        if ( gridCoords[i].xy.x == HUGE_VAL || gridCoords[i].xy.y == HUGE_VAL ) {
            BOOST_LOG_TRIVIAL(error) <<   "Valeurs retournees par pj_transform invalides"  ;
            proj_destroy (pj_conv_normalize);
            return false;
        }
    }

    proj_destroy (pj_conv_normalize);

    BOOST_LOG_TRIVIAL(debug) <<   "Après (centre du pixel en haut à gauche) "<< gridCoords[0].xy.x << " " << gridCoords[0].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Après (centre du pixel en haut à droite) "<< gridCoords[nbx-1].xy.x << " " << gridCoords[nbx-1].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Après (centre du pixel en bas à gauche) "<< gridCoords[nbx*(nby-1)].xy.x << " " << gridCoords[nbx*(nby-1)].xy.y  ;
    BOOST_LOG_TRIVIAL(debug) <<   "Après (centre du pixel en bas à droite) "<< gridCoords[nbx*nby-1].xy.x << " " << gridCoords[nbx*nby-1].xy.y  ;

    /****************** Mise à jour de la bbox *********************
     * On n'utilise pas les coordonnées présentent dans les tableaux X et Y car celles ci correspondent
     * aux centres des pixels, et non au bords. On va donc reprojeter la bbox indépendemment.
     * On divise chaque côté de la bbox en la dimensions la plus grande.
     */
    if ( ! bbox.reproject ( from_crs, to_crs ) ) {
        BOOST_LOG_TRIVIAL(error) <<   "Erreur reprojection bbox"  ;
        return false;
    }

    /****************** Mise à jour du deltaY **********************/
    calculateDeltaY();
    BOOST_LOG_TRIVIAL(debug) <<   "New first line Y-delta :" << deltaY  ;

    return true;
}

int Grid::get_line ( int line, float* X, float* Y ) {

    int dy = line / stepInt;
    double w = 0;

    if ( dy == nbyReg - 1 ) {
        if ( endY == 0 ) {
            w = 0;
        } else {
            w = ( ( line%stepInt ) ) / double ( endY );
        }
    } else {
        w = ( line%stepInt ) / double ( stepInt );
    }

    double LX[nbx], LY[nbx];

    // Interpolation dans les sens des Y
    for ( int i = 0; i < nbx; i++ ) {
        LX[i] = ( 1-w ) * gridCoords[dy*nbx + i].xy.x + w * gridCoords[( dy+1 ) * nbx + i].xy.x ;
        LY[i] = ( 1-w ) * gridCoords[dy*nbx + i].xy.y + w * gridCoords[( dy+1 ) * nbx + i].xy.y ;
    }

    // Indice dans la grille du dernier pixel reprojetée car respecte le pas de base (dans le sens des x)
    int lastRegularPixel = ( nbxReg-1 ) *stepInt;

    /* Interpolation dans le sens des X, sur la partie où la répartition des pixels reprojetés
     * est régulière (tous les stepInt pixels */
    for ( int i = 0; i <= lastRegularPixel; i++ ) {
        int dx = i / stepInt;
        double w = ( i%stepInt ) / double ( stepInt );
        X[i] = ( 1-w ) *LX[dx] + w*LX[dx+1];
        Y[i] = ( 1-w ) *LY[dx] + w*LY[dx+1];
    }

    /* Interpolation dans le sens des X, sur la partie où la distance entre les deux pixels de la grille est différente */
    for ( int i = 1; i <= endX; i++ ) {
        double w = i /double ( endX );
        X[lastRegularPixel + i] = ( 1-w ) *LX[nbxReg - 1] + w * LX[nbxReg];
        Y[lastRegularPixel + i] = ( 1-w ) *LY[nbxReg - 1] + w * LY[nbxReg];
    }

    return width;

}
