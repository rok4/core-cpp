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
 * \file Grid.h
 ** \~french
 * \brief Définition de la classe Grid
 ** \~english
 * \brief Define class Grid
 */

#pragma once

#include "rok4/utils/BoundingBox.h"
#include "rok4/utils/CRS.h"
#include <proj.h>
#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion d'une grille de reprojection
 * \details Une grille est un objet utilisé afin de reprojeter des images (avec ReprojectedImage). On voudra connaître les coordonnées pixel dans l'image source d'un pixel de l'image reprojetée.
 *
 * On imagine donc un quadrillage aux mêmes dimensions que l'image reprojetée (largeur, hauteur et rectangle englobant). On ne va pas convertir tous les pixels de l'image (problème de performance). On va donc en convertir un tous les #pixel_step pixels : c'est ce qu'on considère comme la grille.
 *
 * \~ \image html grid_general.png \~french
 *
 * La grille ne sera pas composée que de ces pixels régulièrement espacés (#x_regular_points sur #y_regular_points). On va également ajouter les pixels du bords : on aura donc en tout (#x_regular_points + 1) x (#y_regular_points + 1) points dans notre grille. Les derniers points d'une ligne ou d'une colonne seront donc moins espacés. Cette distance, strictement inférieure à #pixel_step, sera connue est stockée dans #x_pixels_remainder et #y_pixels_remainder.
 *
 * Dans le cas où l'espacement régulier permet d'avoir le dernier point, on ajoutera tout de même un point supplémentaire. Les deux derniers points de chaque ligne et colonne seront donc identiques. Cela permet d'avoir un comportement plus général.
 *
 * Voici la démarche à suivre pour utiliser une grille pour une reprojection. Imaginons que l'on veuille obtenir une image dans un systéme spatial B à partir d'une image source dans un systéme spatial A
 * \li On crée la grille aux dimensions de l'image reprojetée. On calcule alors les coordonnées de tous les points de la grille, dans le système B.
 * \li On reprojette la grille, c'est à dire que l'on convertit les coordonnées de tous les points de la grille dans le système A.
 * \li À l'aide d'une transformation affine, on convertit les coordonnées dans le système A en coordonnées pixel dans l'image source : on divise par la résolution source et on ramène à l'origine de l'image source (coin supérieur gauche).
 *
 * Cette grille peut enfin être fournie à l'objet ReprojectedImage.
 *
 * \~english \brief Reprojection grid management
 */
class Grid {

private:
    /**
     * \~french \brief Pas (en pixel) de la grille
     * \~english \brief Grid's step, in pixel
     */
    static const int pixel_step = 16;

    /**
     * \~french \brief Ecart maximal entre les coordonnées Y de la première ligne de la grille
     * \details Au fur et à mesure des conversions des points de la grille, on va mettre à jour cette valeur. Celle ci est utilisée pour savoir combien de lignes sources mémoriser dans ReprojectedImage (ReprojectedImage#memorizedLines).
     * \~english \brief Maximal gap for Y-coordinates in the first grid's line
     */
    double y_maximal_gap;

    /**
     * \~french \brief Nombre de points reprojetés, dans les sens des X, respectant le pas régulier
     * \~english \brief Reprojected points number, X wise, respecting the step
     */
    int x_regular_points;

    /**
     * \~french \brief Nombre de points reprojetés, dans les sens des Y, respectant le pas régulier
     * \~english \brief Reprojected points number, Y wise, respecting the step
     */
    int y_regular_points;

    /**
     * \~french \brief Nombre de points reprojetés, dans les sens des X, en tout
     * \~english \brief Reprojected points number, X wise
     */
    int x_points;

    /**
     * \~french \brief Nombre de points reprojetés, dans les sens des Y, en tout
     * \~english \brief Reprojected points number, Y wise
     */
    int y_points;

    /**
     * \~french \brief Nombre de pixels entre le dernier point régulier et le dernier de la grille, dans le sens des X
     * \details Peut être 0.
     * \~english \brief Pixel distance between the last regular point and the last point of the grid, X wise
     * \details Can be null
     */
    int x_pixels_remainder;

    /**
     * \~french \brief Nombre de pixels entre le dernier point régulier et le dernier de la grille, dans le sens des Y
     * \details Peut être 0.
     * \~english \brief Pixel distance between the last regular point and the last point of the grid, Y wise
     * \details Can be null
     */
    int y_pixels_remainder;

    /**
     * \~french \brief Coordonnées des points de la grille
     * \~english \brief Coordinates of the grid's points
     */
    PJ_COORD *grid_coords;

    /**
     * \~french \brief Met à jour la valeur de y_maximal_gap
     * \details À appeler après une conversion apportées aux coordonnées
     * \~english \brief Update the y_maximal_gap value
     */
    inline void compute_y_maximal_gap();

public:

    /**
     * \~french \brief Largeur couverte par la grille
     * \~english \brief Width covered by the grid
     */
    int width;
    /**
     * \~french \brief Hauteur couverte par la grille
     * \~english \brief Height covered by the grid
     */
    int height;

    /**
     * \~french \brief Rectangle englobant couvert par la grille
     * \~english \brief Bounding box covered by the grid
     */
    BoundingBox<double> bbox;

    /**
     * \~french \brief Crée un objet Grid à partir des dimensions à couvrir
     * \param[in] width largeur à couvrir
     * \param[in] height hauteur à couvrir
     * \param[in] bbox emprise rectangulaire à couvrir
     * \~english \brief Create a Grid object, from dimensions to cover
     * \param[in] width width to cover
     * \param[in] height height to cover
     * \param[in] bbox bounding box to cover
     */
    Grid ( int width, int height, BoundingBox<double> bbox );

    /**
     * \~french \brief Destructeur par défaut
     * \details Suppression du tableau #grid_coords
     * \~english \brief Default destructor
     * \details Delete array #grid_coords
     */
    ~Grid() {
        delete[] grid_coords;
    }

    /**
     * \~french \brief Retourne la valeur de y_maximal_gap
     * \return #y_maximal_gap
     * \~english \brief Return the y_maximal_gap value
     * \return #y_maximal_gap
     */
    double get_y_maximal_gap() {
        return y_maximal_gap;
    }

    /**
     * \~french \brief Retourne le ratio dans le sens des X
     * \details Le ratio dans le sens des X est une pseudo résolution : c'est la différence entre les valeurs en bout de ligne, divisée par la largeur #width. Ce calcul est effectué pour chaque ligne, et on conserve la valeur la plus grande.
     * \return ratio X
     * \~english \brief Return the X wise ratio
     * \return X ratio
     */
    double get_x_ratio();

    /**
     * \~french \brief Retourne le ratio dans le sens des Y
     * \details Le ratio dans le sens des Y est une pseudo résolution : c'est la différence entre les valeurs en bout de colonne, divisée par la hauteur #height. Ce calcul est effectué pour chaque colonne, et on conserve la valeur la plus grande.
     * \return ratio Y
     * \~english \brief Return the Y wise ratio
     * \return Y ratio
     */
    double get_y_ratio();

    /**
     * \~french \brief Reprojette les points de la grille
     * \details On fera particulièrement attentiotn à ce que les points de la grille appartiennent bien à la zone de définition du système spatial.
     * \param[in] from_crs système spatial source, celui de la grille initialement
     * \param[in] to_crs système spatial de destination, celui dans lequel on veut la grille
     * \return VRAI si succès, FAUX sinon.
     * \~english \brief Reproject grid's points
     */
    bool reproject ( CRS* from_crs, CRS* to_crs );

    /**
     * \~french \brief Applique une transformation affine
     * \details Tous les points de la grille subisse la transformation affine suivante :
     * \li X = Ax x X + Bx
     * \li Y = Ay x Y + By
     *
     * On met à jour #y_maximal_gap en le multipliant par la valeur absolue de Ay
     * \param[in] Ax homothétie en X
     * \param[in] Bx translation en X
     * \param[in] Ay homothétie en Y
     * \param[in] By translation en Y
     * \~english \brief Apply an affine transformation
     */
    void affine_transform ( double Ax, double Bx, double Ay, double By );

    /**
     * \~french \brief Retourne une ligne de la grille, complétée par interpolation linéaire
     * \details On veut une ligne (entre 0 et #height) de largeur #width. Seulement certains points ont été calculés (reprojetection,transformation affine). On va donc interpoler linéairement les valeurs entres les points de la grille.
     * \param[in] line indice de la ligne voulue
     * \param[in,out] X buffer dans lequel stocker les abscisses de la ligne
     * \param[in,out] Y buffer dans lequel stocker les ordonnées de la ligne
     * \~english \brief Return a grid's line, completed by linear interpolation
     */
    int get_line ( int line, float* X, float* Y );

    /** \~french
     * \brief Sortie des informations sur la grille de reprojection
     ** \~english
     * \brief Resampled image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "\t--------- Grid -----------" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Size : " << width << ", " << height ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- BBOX : " << bbox.to_string() ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Reprojected points number :" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- X wise : " << x_points ;
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- Y wise : " << y_points ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Last point distance :" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- X : " << x_pixels_remainder ;
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- Y : " << y_pixels_remainder ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- First line Y-delta :" << y_maximal_gap ;
    }

};


