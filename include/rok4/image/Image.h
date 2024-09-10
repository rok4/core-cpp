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
 * \file Image.h
 * \~french
 * \brief Définition de la classe abstraite Image, abstrayant les différents types d'images.
 * \~english
 * \brief Define the Image abstract class , to abstract all kind of images.
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <typeinfo>
#include <cmath>

#include "rok4/utils/BoundingBox.h"
#include "rok4/utils/CRS.h"
#include "rok4/utils/Cache.h"

#define METER_PER_DEG 111319.492

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Interface de manipulation d'images
 * \details Cette classe abstraite permet d'utiliser toutes les images de la même manière, en assurant les définitions des fonctions permettant de lire une ligne, connaître l'emprise géographique de l'image... Toutes ces informations que l'on veut connaître sur une image, qu'elle soit directement liée à un fichier, ou le réechantillonnage d'un ensemble d'images.
 *
 * On permet l'association d'un masque de donnée, qui n'est autre qu'une image à un canal sur 8 bits qui permet la distinction entre un pixel de l'image qui contient de la vraie donnée de celui qui contient du nodata.
 *
 * Ne sont gérés que les formats suivant pour les canaux :
 * \li flottant sur 32 bits
 * \li entier non signé sur 8 bits
 *
 * Le géoréférencement est assuré par le renseignement des résolutions et du rectangle englobant. Cependant, on peut également gérer des images simples. Dans ce cas, on mettra par convention une bbox à 0,0,0,0 et des résolutions à -1. Aucun test ne sera fait par les fonctions qui utilisent ces attributs. On doit donc bien faire attention à rester cohérent.
 */
class Image {

protected:

    /**
     * \~french \brief Nombre de canaux de l'image
     * \~english \brief Number of samples per pixel
     */
    int channels;
    /**
     * \~french \brief Largeur de l'image en pixel
     * \~english \brief Image's width, in pixel
     */
    int width;

    /**
     * \~french \brief Hauteur de l'image en pixel
     * \~english \brief Image's height, in pixel
     */
    int height;

    /**
     * \~french \brief L'image est-ell un masque ?
     * \~english \brief Is this image a mask ?
     */
    bool is_mask;

    /**
     * \~french \brief Emprise rectangulaire au sol de l'image
     * \~english \brief Image's bounding box
     */
    BoundingBox<double> bbox;

    /**
     * \~french \brief CRS du rectangle englobant de l'image
     * \~english \brief Bounding box's CRS
     */
    CRS* crs;

    /**
     * \~french \brief Masque de donnée associé à l'image, facultatif
     * \~english \brief Mask associated to the image, optionnal
     */
    Image* mask;

    /**
     * \~french \brief Resolution de l'image, dans le sens des X
     * \~english \brief Image's resolution, X wise
     */
    double resx;
    /**
     * \~french \brief Resolution de l'image, dans le sens des Y
     * \~english \brief Image's resolution, Y wise
     */
    double resy;

    /**
     * \~french
     * \brief Calcul des resolutions en x et en y, calculées à partir des dimensions et de la bouding box
     * \~english
     * \brief Resolutions calculation, from pixel size and bounding box
     */
    void compute_resolutions() {
        resx = ( bbox.xmax - bbox.xmin ) / double ( width );
        resy = ( bbox.ymax - bbox.ymin ) / double ( height );
    }

public:
    
    /**
     * \~french
     * \brief Vérifie la cohérence des dimensions d'image fournies
     * \param[in] resolution_x Resolution de l'image, dans le sens des X
     * \param[in] resolution_y Resolution de l'image, dans le sens des Y
     * \param[in] w Largeur de l'image en pixel
     * \param[in] h Hauteur de l'image en pixel
     * \param[in] bounding_box Emprise rectangulaire au sol de l'image
     * \~english
     * \brief Check provided dimensions' consistency
     * \param[in] resolution_x Image's resolution, X wise
     * \param[in] resolution_y Image's resolution, Y wise
     * \param[in] w Image's width, in pixel
     * \param[in] h Image's height, in pixel
     * \param[in] bounding_box Image's bounding box
     */
    static bool are_dimensions_consistent(double resolution_x, double resolution_y, int w, int h, BoundingBox<double> bounding_box) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche

        if (resolution_x == 0 || resolution_y == 0) return false;

        int calcWidth = lround ( ( bounding_box.xmax - bounding_box.xmin ) / ( resolution_x ) );
        int calcHeight = lround ( ( bounding_box.ymax - bounding_box.ymin ) / ( resolution_y ) );

        if ( calcWidth != w || calcHeight != h ) {
            BOOST_LOG_TRIVIAL(warning) <<  "height is " << h << " and calculation give " << calcHeight ;
            BOOST_LOG_TRIVIAL(warning) <<  "width is " << w << " and calculation give " << calcWidth ;
            return false;
        }
        
        return true;
    }

    /**
     * \~french
     * \brief Définit l'image comme un masque
     * \~english
     * \brief Define the image as a mask
     */
    inline void make_mask () {
        is_mask = true;
    }

    /**
     * \~french
     * \brief Retourne le nombre de canaux par pixel
     * \return channels
     * \~english
     * \brief Return the number of samples per pixel
     * \return channels
     */
    virtual int get_channels() {
        return channels;
    }

    /**
     * \~french
     * \brief Retourne la largeur en pixel de l'image
     * \return largeur
     * \~english
     * \brief Return the image's width
     * \return width
     */
    int inline get_width() {
        return width;
    }

    /**
     * \~french
     * \brief Retourne la hauteur en pixel de l'image
     * \return hauteur
     * \~english
     * \brief Return the image's height
     * \return height
     */
    int inline get_height() {
        return height;
    }

    /**
     * \~french
     * \brief Définit l'emprise rectangulaire de l'image et calcule les résolutions
     * \param[in] box Emprise rectangulaire de l'image
     * \~english
     * \brief Define the image's bounding box and calculate resolutions
     * \param[in] box Image's bounding box
     */
    inline void set_bbox ( BoundingBox<double> box ) {
        bbox = box;
        if (crs != NULL) {
            bbox.crs = crs->get_request_code();
        }
        compute_resolutions();
    }

    /**
     * \~french
     * \brief Définit les dimensions de l'image, en vérifiant leurs cohérences
     * \param[in] box Emprise rectangulaire de l'image
     * \~english
     * \brief Define the image's bounding box and calculate resolutions
     * \param[in] box Image's bounding box
     */
    inline bool set_dimensions ( int w, int h, BoundingBox<double> box, double rx, double ry ) {
        double calcWidth = (box.xmax - box.xmin) / rx;
        double calcHeight = (box.ymax - box.ymin) / ry;
        
        if ( std::abs(calcWidth - w) > 10E-3 || std::abs(calcHeight - h) > 10E-3) return false;
        
        width = w;
        height = h;
        resx = rx;
        resy = ry;
        bbox = box;

        return true;
    }

    /**
     * \~french
     * \brief Retourne l'emprise rectangulaire de l'image
     * \return emprise
     * \~english
     * \brief Return the image's bounding box
     * \return bounding box
     */
    BoundingBox<double> inline get_bbox() const {
        return bbox;
    }

    /**
     * \~french
     * \brief Définit le SRS de l'emprise rectangulaire
     * \details On clone bien le CRS pour ne pas avoir de conflit lors du nettoyage
     * \param[in] box Emprise rectangulaire de l'image
     * \~english
     * \brief Define the CRS of the image's bounding box
     * \param[in] box Image's bounding box
     */
    void set_crs ( CRS* c ) {
        crs = NULL;

        if (c != NULL) {
            crs = CrsBook::get_crs(c->get_request_code());
            bbox.crs = crs->get_request_code();
        } else {
            bbox.crs = "";
        }
    }

    /**
     * \~french
     * \brief Retourne le SRS de l'emprise rectangulaire de l'image
     * \return CRS
     * \~english
     * \brief Return the image's bounding box's CRS
     * \return CRS
     */
    CRS* get_crs() const {
        return crs;
    }

    /**
     * \~french
     * \brief Retourne le xmin de l'emprise rectangulaire
     * \return xmin
     * \~english
     * \brief Return bounding box's xmin
     * \return xmin
     */
    double inline get_xmin() const {
        return bbox.xmin;
    }
    /**
     * \~french
     * \brief Retourne le ymax de l'emprise rectangulaire
     * \return ymax
     * \~english
     * \brief Return bounding box's ymax
     * \return ymax
     */
    double inline get_ymax() const {
        return bbox.ymax;
    }
    /**
     * \~french
     * \brief Retourne le xmax de l'emprise rectangulaire
     * \return xmax
     * \~english
     * \brief Return bounding box's xmax
     * \return xmax
     */
    double inline get_xmax() const {
        return bbox.xmax;
    }
    /**
     * \~french
     * \brief Retourne le ymin de l'emprise rectangulaire
     * \return ymin
     * \~english
     * \brief Return bounding box's ymin
     * \return ymin
     */
    double inline get_ymin() const {
        return bbox.ymin;
    }

    /**
     * \~french
     * \brief Retourne la résolution dans le sens des X
     * \param[in] force_meter Cherche à convertir en mètre si besoin
     * \return résolution en X
     * \~english
     * \brief Return the X wise resolution
     * \param[in] force_meter Convert to meter according to CRS
     * \return X resolution
     */
    inline double get_resx(bool force_meter = false) const {
        if (force_meter && crs->get_meters_per_unit() != 1.0) return resx * METER_PER_DEG;
        return resx;
    }
    /**
     * \~french
     * \brief Retourne la résolution dans le sens des Y
     * \param[in] force_meter Cherche à convertir en mètre si besoin
     * \return résolution en Y
     * \~english
     * \brief Return the Y wise resolution
     * \param[in] force_meter Convert to meter according to CRS
     * \return Y resolution
     */
    inline double get_resy(bool force_meter = false) const {
        if (force_meter && crs->get_meters_per_unit() != 1.0) return resy * METER_PER_DEG;
        return resy;
    }

    /**
     * \~french
     * \brief Retourne le masque de donnée associé à l'image
     * \return masque
     * \~english
     * \brief Return the associated mask
     * \return mask
     */
    inline Image* get_mask() {
        return mask;
    }

    /**
     * \~french
     * \brief Définit le masque de donnée et contrôle la cohérence avec l'image
     * \param[in] new_mask Masque de donnée
     * \~english
     * \brief Defined data mask and check consistency
     * \param[in] new_mask Masque de donnée
     */
    inline bool set_mask ( Image* new_mask ) {
        if (mask != NULL) {
            // On a déjà un masque associé : on le supprime pour le remplacer par le nouveau
            delete mask;
        }
        
        if ( new_mask->get_width() != width || new_mask->get_height() != height || new_mask->get_channels() != 1 ) {
            BOOST_LOG_TRIVIAL(error) <<   "Unvalid mask"  ;
            BOOST_LOG_TRIVIAL(error) <<   "\t - channels have to be 1, it is " << new_mask->get_channels()  ;
            BOOST_LOG_TRIVIAL(error) <<   "\t - width have to be " << width << ", it is " << new_mask->get_width()  ;
            BOOST_LOG_TRIVIAL(error) <<   "\t - height have to be " << height << ", it is " << new_mask->get_height()  ;
            return false;
        }

        mask = new_mask;
        mask->make_mask();

        return true;
    }

    /**
     * \~french
     * \brief Conversion de l'abscisse terrain vers l'indice de colonne dans l'image
     * \param[in] x abscisse terrain
     * \return colonne
     * \~english
     * \brief Conversion from terrain coordinate X to image column indice
     * \param[in] x terrain coordinate X
     * \return column
     */
    int inline x_to_column ( double x ) {
        return floor ( ( x-bbox.xmin ) /resx );
    }
    /**
     * \~french
     * \brief Conversion de l'ordonnée terrain vers l'indice de ligne dans l'image
     * \param[in] y ordonnée terrain
     * \return ligne
     * \~english
     * \brief Conversion from terrain coordinate Y to image line indice
     * \param[in] y terrain coordinate Y
     * \return line
     */
    int inline y_to_line ( double y ) {
        return floor ( ( bbox.ymax-y ) /resy );
    }

    /**
     * \~french
     * \brief Conversion de l'indice de colonne dans l'image vers l'abscisse terrain du centre du pixel
     * \param[in] c colonne
     * \return abscisse terrain
     * \~english
     * \brief Conversion from image column indice to terrain coordinate X (pixel's center)
     * \param[in] c column
     * \return terrain coordinate X
     */
    double inline column_to_x ( int c ) {
        return ( bbox.xmin + (0.5 + c) * resx );
    }
    /**
     * \~french
     * \brief Conversion de l'indice de ligne dans l'image vers l'ordonnée terrain du centre du pixel
     * \param[in] l ligne
     * \return ordonnée terrain
     * \~english
     * \brief Conversion from image line indice to terrain coordinate X (pixel's center)
     * \param[in] l line
     * \return terrain coordinate Y
     */
    double inline line_to_y ( int l ) {
        return ( bbox.ymax - (0.5 + l) * resy );
    }

    /**
     * \~french
     * \brief Calcul de la phase dans le sens des X
     * \details La phase en X est le décalage entre le bord gauche du pixel et le 0 des abscisses, évalué en pixel. On a donc un nombre décimal appartenant à [0,1[.
     * \image html phases.png
     * \return phase X
     * \~english
     * \brief Phasis calculation, X wise
     * \image html phases.png
     * \return X phasis
     */
    double inline get_phasex() {
        return bbox.get_xmin_phase(resx);
    }

    /**
     * \~french
     * \brief Calcul de la phase dans le sens des Y
     * \details La phase en Y est le décalage entre le bord haut du pixel et le 0 des ordonnées, évalué en pixel. On a donc un nombre décimal appartenant à [0,1[.
     * \return phase Y
     * \~english
     * \brief Phasis calculation, Y wise
     * \return Y phasis
     */
    double inline get_phasey() {
        return bbox.get_ymin_phase(resy);
    }

    /**
     * \~french
     * \brief Détermine la compatibilité avec une autre image, en comparant phases et résolutions
     * \details On parle d'images compatibles lorsqu'elles ont :
     * \li le même SRS
     * \li la même résolution en X
     * \li la même résolution en Y
     * \li la même phase en X
     * \li la même phase en Y
     * \li le même nombre de canaux
     *
     * Les tests d'égalité acceptent un epsilon qui est le suivant :
     * \li 1 pour mille de la résolution la plus petite pour les résolutions
     * \li 1 pour mille pour les phases
     *
     * \param[in] other image à comparer
     * \return compatibilité
     * \~english
     * \brief Determine compatibility with another image, comparing CRS, phasis and resolutions and channels
     * \param[in] other image to compare
     * \return compatibility
     */
    bool compatible ( Image* other ) {

        if ( crs != NULL && crs->is_define() && other->get_crs() != NULL && other->get_crs()->is_define() && ! crs->cmp_request_code(other->get_crs()->get_request_code()) ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different CRS"  ;
            return false;
        }

        if ( get_channels() != other->get_channels() ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different channels"  ;
            return false;
        }

        double epsilon_x=std::min ( get_resx(), other->get_resx() ) /1000.;
        double epsilon_y=std::min ( get_resy(), other->get_resy() ) /1000.;

        if ( fabs ( get_resx()-other->get_resx() ) > epsilon_x ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different X resolutions"  ;
            return false;
        }
        if ( fabs ( get_resy()-other->get_resy() ) > epsilon_y ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different Y resolutions"  ;
            return false;
        }

        if ( fabs ( get_phasex()-other->get_phasex() ) > 0.001 && fabs ( get_phasex()-other->get_phasex() ) < 0.999 ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different X phasis : " << get_phasex() << " and " << other->get_phasex()  ;
            return false;
        }
        if ( fabs ( get_phasey()-other->get_phasey() ) > 0.001 && fabs ( get_phasey()-other->get_phasey() ) < 0.999 ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Different Y phasis : " << get_phasey() << " and " << other->get_phasey()  ;
            return false;
        }

        return true;
    }

    /**
     * \~french
     * \brief Crée un objet Image à partir de tous ses éléments constitutifs
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] bbox emprise rectangulaire de l'image
     * \~english
     * \brief Create an Image object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     */
    Image ( int width, int height, int channels, double resx, double resy,  BoundingBox<double> bbox ) :
        width ( width ), height ( height ), channels ( channels ), resx ( resx ), resy ( resy ), bbox ( bbox ), crs ( NULL ), mask ( NULL ), is_mask(false)
    {
        are_dimensions_consistent(resx, resy, width, height, bbox);
    }

    /**
     * \~french
     * \brief Crée une Image sans préciser de géoréférencement, ni résolutions, ni rectangle englobant
     * \details La résolution sera de 1 dans les 2 sens et le rectangle englobant sera 0,0 width,height
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \~english
     * \brief Create an Image without providing georeferencement, neither resolutions nor bounding box
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     */
    Image ( int width, int height, int channels ) : width ( width ), height ( height ), channels ( channels ), resx ( 1. ), resy ( 1. ), bbox ( BoundingBox<double> ( 0., 0., ( double ) width, ( double ) height ) ), crs ( NULL ), mask ( NULL ), is_mask ( false ) {}

    /**
     * \~french
     * \brief Crée une Image sans préciser les résolutions
     * \details Les résolutions sont calculées à partie du rectangle englobant et des dimensions en pixel.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \~english
     * \brief Create an Image without providing resolutions
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     */
    Image ( int width, int height, int channels,  BoundingBox<double> bbox ) :
        width ( width ), height ( height ), channels ( channels ), bbox ( bbox ), crs ( NULL ), mask ( NULL ), is_mask ( false )
    {
        compute_resolutions();
    }

    /**
     * \~french
     * \brief Crée un objet Image sans préciser le rectangle englobant
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \~english
     * \brief Create an Image object without providing bbox
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     */
    Image ( int width, int height, int channels, double resx, double resy ) :
        width ( width ), height ( height ), channels ( channels ), resx ( resx ), resy ( resy ),
        bbox ( BoundingBox<double> ( 0., 0., resx * ( double ) width, resy * ( double ) height ) ),
        mask ( NULL ), is_mask ( false ), crs ( NULL ) {}

    /**
     * \~french
     * \brief Retourne une ligne en entier 8 bits.
     * Les canaux sont entrelacés. ATTENTION : si les données ne sont pas intrinsèquement codées sur des entiers 8 bits, il n'y a pas de conversion (une valeur sur 32 bits occupera 4 "cases" sur 8 bits).
     * \param[in,out] buffer Tableau contenant au moins 'width * channels * sizeof(sample)' entier sur 8 bits
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    virtual int get_line ( uint8_t *buffer, int line ) = 0;
    
    /**
     * \~french
     * \brief Retourne une ligne en entier 16 bits.
     * Les canaux sont entrelacés. ATTENTION : si les données ne sont pas intrinsèquement codées sur des entiers 16 bits, il n'y a pas de conversion (une valeur sur 32 bits occupera 2 "cases" sur 16 bits).
     * \param[in,out] buffer Tableau contenant au moins 'width * channels * sizeof(sample)' entier sur 16 bits
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    virtual int get_line ( uint16_t *buffer, int line ) = 0;

    /**
     * \~french
     * \brief Retourne une ligne en flottant 32 bits.
     * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des flottants 32 bits, une conversion est effectuée.
     * \param[in,out] buffer Tableau contenant au moins 'width * channels' flottant sur 32 bits
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    virtual int get_line ( float *buffer, int line ) = 0;

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Image() {
        if ( mask != NULL ) delete mask;
    }

    /**
     * \~french
     * \brief Sortie des informations sur l'image
     * \~english
     * \brief Image description output
     */
    virtual void print() {
        BOOST_LOG_TRIVIAL(info) <<   "\t- width = " << width << ", height = " << height  ;
        BOOST_LOG_TRIVIAL(info) <<   "\t- samples per pixel = " << channels  ;
        if (crs != NULL) {
            BOOST_LOG_TRIVIAL(info) <<   "\t- CRS = " << crs->get_proj_code()  ;
        } else {
            BOOST_LOG_TRIVIAL(info) <<   "\t- No CRS"  ;
        }
        BOOST_LOG_TRIVIAL(info) <<   "\t- bbox = " << bbox.to_string()  ;
        BOOST_LOG_TRIVIAL(info) <<   "\t- x resolution = " << resx << ", y resolution = " << resy  ;
        if ( is_mask ) {
            BOOST_LOG_TRIVIAL(info) <<   "\t- Is a mask"  ;
        } else {
            BOOST_LOG_TRIVIAL(info) <<   "\t- Is not a mask"  ;
        }
        if ( mask ) {
            BOOST_LOG_TRIVIAL(info) <<   "\t- Own a mask\n"  ;
        } else {
            BOOST_LOG_TRIVIAL(info) <<   "\t- No mask\n"  ;
        }
    }
    
    /**
     * \~french
     * \brief Sortie du tfw de l'image
     * \~english
     * \brief Image TFW output
     */
    virtual void print_tfw() {
        BOOST_LOG_TRIVIAL(info) <<   "TFW : \n" << resx << "\n-" << resy << "\n0\n0\n" << bbox.xmin+0.5*resx << "\n" << bbox.ymax - 0.5*resy  ;
    }

    /**
     * \~french
     * \brief Résolution moyenne
     * \~english
     * \brief Mean resolution
     */
    virtual float get_mean_resolution() {
        if (crs->get_meters_per_unit() != 1.0) {
            return ((resx+resy)/2.0)*METER_PER_DEG;
        } else {
            return (resx+resy)/2.0;
        }

    }
};


