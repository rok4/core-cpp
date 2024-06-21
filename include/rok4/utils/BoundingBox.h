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
 * \file BoundingBox.h
 ** \~french
 * \brief Définition de la classe template BoundingBox
 ** \~english
 * \brief Define template class BoundingBox
 */

class CRS;

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <boost/log/trivial.hpp>
#include <proj.h>
#include <sstream>
#include <cmath>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion d'un rectangle englobant
 * \details Cette classe template gère des coordonnées de plusieurs types, avec des constructeurs avec conversion. Elle met également à disposition des fonctions de reprojection et topologiques.
 * \~english \brief Manage a bounding box
 */
template<typename T>
class BoundingBox {

public:
    /**
     * \~french \brief Extrema du rectangle englobant
     * \~english \brief Bounding box limits
     */
    T xmin, ymin, xmax, ymax;

    /**
     * \~french \brief Système de coordonnées de la bbox
     * \~english \brief Bbox CRS
     */
    std::string crs;

    /** \~french
     * \brief Constructeur par défaut
     ** \~english
     * \brief Default constructor
     */
    BoundingBox ( ) :
        xmin ( 0 ), ymin ( 0 ), xmax ( 0 ), ymax ( 0 ), crs ("") {}

    /** \~french
     * \brief Crée un objet BoundingBox à partir de tous ses éléments constitutifs
     ** \~english
     * \brief Create a BoundingBox object, from all attributes
     */
    BoundingBox ( T xmin, T ymin, T xmax, T ymax ) :
        xmin ( xmin ), ymin ( ymin ), xmax ( xmax ), ymax ( ymax ), crs ("") {}

    /** \~french \brief Crée un objet BoundingBox par copie et conversion
     * \param[in] bbox rectangle englobant à copier et éventuellement convertir
     ** \~english \brief Create a BoundingBox object, copying and converting
     * \param[in] bbox bounding box to copy and possibly convert
     */
    template<typename T2>
    BoundingBox ( const BoundingBox<T2>& bbox ) :
        xmin ( ( T ) bbox.xmin ), ymin ( ( T ) bbox.ymin ), xmax ( ( T ) bbox.xmax ), ymax ( ( T ) bbox.ymax ), crs (bbox.crs) {}

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~BoundingBox() {}

    
    bool isInAreaOfCRS(CRS* c);

    bool intersectAreaOfCRS(CRS* c) ;

    BoundingBox<T> cropToAreaOfCRS ( CRS* c ) ;

    /** \~french
     * \brief Reprojette le rectangle englobant (SRS sous forme de chaîne de caractères)
     * \details Pour reprojeter la bounding box, on va découper chaque côté du rectangle en N, et identifier les extrema parmi ces 4*N points reprojetés.
     * \param[in] from_srs système spatial source, celui du rectangle englobant initialement
     * \param[in] to_srs système spatial de destination, celui dans lequel on veut le rectangle englobant
     * \param[in] nbSegment nombre de points intérmédiaire à reprojeter sur chaque bord. 256 par défaut.
     * \return code de retour, vrai si succès, faux sinon.
     */
    bool reproject ( CRS* from_crs, CRS* to_crs , int nbSegment = 256 );

    /** \~french \brief Sortie des informations sur le rectangle englobant
     ** \~english \brief Bounding box description output
     */
    void print() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << xmin << "," << ymin << "," << xmax << "," << ymax;
        BOOST_LOG_TRIVIAL(debug) <<  "BBOX (" << crs << ") = " << oss.str() ;
    }

    /** \~french \brief Conversion des informations sur le rectangle englobant en string
     * \return chaîne de carcactère décrivant le rectangle englobant
     ** \~english \brief Convert bounding box description to string
     * \return string describing the bounding box
     */
    std::string to_string() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << xmin << "," << ymin << "," << xmax << "," << ymax;
        return oss.str() ;
    }

    /** \~french \brief Détermine si deux bounding box s'intersectent
     * \details On considère les bbox comme étant dans le même CRS
     * \param[in] bbox rectangle englobant avec lequel tester l'intersection
     ** \~english \brief Determine if 2 bounding box intersect each other
     * \param[in] bbox bounding box which whom we have to test intersection
     */
    bool intersects ( BoundingBox<T> bbox ) {
        return ! ( xmax < bbox.xmin || bbox.xmax < xmin || ymax < bbox.ymin || bbox.ymax < ymin );
    }

    /** \~french \brief Détermine si une bounding box contient l'autre
     * \details On considère les bbox comme étant dans le même CRS
     * \param[in] bbox rectangle englobant dont on veut savoir s'il est contenu dans l'autre
     ** \~english \brief Determine if a bounding box contains the other or touch inside
     * \param[in] bbox bounding box : is it contained by the other ?
     */
    bool contains ( BoundingBox<T> bbox ) {
        return ( xmin <= bbox.xmin && bbox.xmax <= xmax && ymin <= bbox.ymin && bbox.ymax <= ymax );
    }

    /** \~french \brief Récupère la partie utile de la bbox qui appelle la fonction, en fonction de la bbox en parametre
     * ATTENTION: on part du principe que les bbox sont dans le même CRS
     * \param[in] bbox
     ** \~english \brief Get the useful part of the bbox which call the function, depending of the parameter bbox
     ** WARNING: the two bbox must have the same CRS
     * \param[in] bbox
     */
    BoundingBox<T> getIntersection ( BoundingBox<T> other ) {

        if (! this->intersects(other)) {
            return BoundingBox<T> (0,0,0,0);
        }

        return BoundingBox<T> (
            std::max(this->xmin, other.xmin),
            std::max(this->ymin, other.ymin),
            std::min(this->xmax, other.xmax),
            std::min(this->ymax, other.ymax)
        );

    }

    /** \~french \brief Retourne l'union de la bbox avec celle en paramètre
     * ATTENTION: on part du principe que les bbox sont dans le même CRS
     * \param[in] bbox
     ** \~english \brief Get the union of the bbox with the provided one
     ** WARNING: the two bbox must have the same CRS
     * \param[in] bbox
     */
    BoundingBox<T> getUnion ( BoundingBox<T> other ) {

        return BoundingBox<T> (
            std::min(this->xmin, other.xmin),
            std::min(this->ymin, other.ymin),
            std::max(this->xmax, other.xmax),
            std::max(this->ymax, other.ymax)
        );

    }

    /**
     * \~french
     * \brief Calcul de la phase de X min
     * \details La phase de Xmin est le décalage entre le bord gauche de la bbox le 0 des abscisses, évalué dans la résolution donnée. On a donc un nombre décimal appartenant à [0,1[.
     * \param[in] res Résolution, unité de quantification de la phase
     * \return phase X min
     * \~english
     * \brief X min phasis calculation
     * \param[in] res Resolution, unity to quantify phase
     * \return X min phasis
     */
    double getPhaseXmin(T res) {
        double intpart;
        double phi = modf ( xmin/res, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /**
     * \~french
     * \brief Calcul de la phase de X max
     * \details La phase de Xmax est le décalage entre le bord droit de la bbox le 0 des abscisses, évalué dans la résolution donnée. On a donc un nombre décimal appartenant à [0,1[.
     * \param[in] res Résolution, unité de quantification de la phase
     * \return phase X max
     * \~english
     * \brief X max phasis calculation
     * \param[in] res Resolution, unity to quantify phase
     * \return X max phasis
     */
    double getPhaseXmax(T res) {
        double intpart;
        double phi = modf ( xmax/res, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /**
     * \~french
     * \brief Calcul de la phase de Y min
     * \details La phase de Ymin est le décalage entre le bord bas de la bbox le 0 des ordonnées, évalué dans la résolution donnée. On a donc un nombre décimal appartenant à [0,1[.
     * \param[in] res Résolution, unité de quantification de la phase
     * \return phase Y min
     * \~english
     * \brief Y min phasis calculation
     * \param[in] res Resolution, unity to quantify phase
     * \return Y min phasis
     */
    double getPhaseYmin(T res) {
        double intpart;
        double phi = modf ( ymin/res, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /**
     * \~french
     * \brief Calcul de la phase de Y max
     * \details La phase de Ymax est le décalage entre le bord haut de la bbox le 0 des ordonnées, évalué dans la résolution donnée. On a donc un nombre décimal appartenant à [0,1[.
     * \param[in] res Résolution, unité de quantification de la phase
     * \return phase Y max
     * \~english
     * \brief Y max phasis calculation
     * \param[in] res Resolution, unity to quantify phase
     * \return Y max phasis
     */
    double getPhaseYmax(T res) {
        double intpart;
        double phi = modf ( ymax/res, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /** \~french \brief Mettre en phase une bbox
     * \details La bbox finale sera toujours plus petite que celle initiale
     ** \~english \brief Phase a bbox
     */
    void phase(BoundingBox<T> other, T resx, T resy) {

        double intpart;
        double phi = 0;
        double phaseDiff = 0;

        // phase de la bbox fournie, de référence, sur laquelle se caser
        // la bbox en entrée est en accord avec les résolutions fournies,
        // c'est à dire que la phase est la même pour le min et le max
        // ce n'est pas forcément le cas pour la bbox à mettre en phase
        double phaseX = other.getPhaseXmin(resx);
        double phaseY = other.getPhaseYmin(resy);

        // Mise en phase de xmin (sans que celui ci puisse être plus petit)
        phi = getPhaseXmin(resx);

        if ( fabs ( phi-phaseX ) > 0.0001 && fabs ( phi-phaseX ) < 0.9999 ) {
            phaseDiff = phaseX - phi;
            if ( phaseDiff < 0. ) {
                phaseDiff += 1.0;
            }
            xmin += phaseDiff * resx;
        }

        // Mise en phase de xmax (sans que celui ci puisse être plus grand)
        phi = getPhaseXmax(resx);

        if ( fabs ( phi-phaseX ) > 0.0001 && fabs ( phi-phaseX ) < 0.9999 ) {
            phaseDiff = phaseX - phi;
            if ( phaseDiff > 0. ) {
                phaseDiff -= 1.0;
            }
            xmax += phaseDiff*resx;
        }

        // Mise en phase de ymin (sans que celui ci puisse être plus petit)
        phi = getPhaseYmin(resy);

        if ( fabs ( phi-phaseY ) > 0.0001 && fabs ( phi-phaseY ) < 0.9999 ) {
            phaseDiff = phaseY - phi;
            if ( phaseDiff < 0. ) {
                phaseDiff += 1.0;
            }
            ymin += phaseDiff*resy;
        }

        // Mise en phase de ymax (sans que celui ci puisse être plus grand)
        phi = getPhaseYmax(resy);

        if ( fabs ( phi-phaseY ) > 0.0001 && fabs ( phi-phaseY ) < 0.9999 ) {
            phaseDiff = phaseY - phi;
            if ( phaseDiff > 0. ) {
                phaseDiff -= 1.0;
            }
            ymax += phaseDiff*resy;
        }

    }

    /** \~french \brief Agrandit une bbox
     ** \~english \brief Expand a bbox
     */
    BoundingBox<T> expand (T resolutionX, T resolutionY, int gap) {
        BoundingBox<T> expanded = *this;
        expanded.xmin = expanded.xmin - gap * resolutionX;
        expanded.xmax = expanded.xmax + gap * resolutionX;
        expanded.ymin = expanded.ymin - gap * resolutionY;
        expanded.ymax = expanded.ymax + gap * resolutionY;
        return expanded;
    }

    /** \~french \brief Détermine si une boundingBox est égale à une autre
     * \param[in] bbox
     ** \~english \brief Determine if a bounding box is equal to an other
     * \param[in] bbox
     */
    bool isEqual ( BoundingBox<T> bbox ) {
        if (crs != "" && bbox.crs != "" && crs != bbox.crs) {
            return false;
        } else {
            return ( xmin == bbox.xmin && bbox.xmax == xmax && ymin == bbox.ymin && bbox.ymax == ymax );
        }
    }

    /** \~french \brief Détermine si une boundingBox est nulle
     ** \~english \brief Determine if a bounding box is null
     */
    bool isNull ( ) {
        return ( xmin == 0 && xmax == 0 && ymin == 0 && ymax == 0 );
    }

    /** \~french \brief Détermine si une boundingBox a une aire nulle
     ** \~english \brief Determine if a bounding box has null area
     */
    bool hasNullArea ( ) {
        return ( xmin >= xmax || ymin >= ymax );
    }

};

#endif

