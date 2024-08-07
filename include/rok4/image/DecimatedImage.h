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
 * \file DecimatedImage.h
 ** \~french
 * \brief Définition des classes DecimatedImage
 * \details
 * \li DecimatedImage : image résultant de la décimation d'une image source
 ** \~english
 * \brief Define classes DecimatedImage
 * \details
 * \li DecimatedImage : image built by source image's decimation
 */

#ifndef DECIMATED_IMAGE_H
#define DECIMATED_IMAGE_H

#include <vector>
#include <cstring>
#include <iostream>
#include <math.h>
#include <boost/log/trivial.hpp>

#include "rok4/utils/Utils.h"
#include "rok4/image/Image.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Décimation d'une image, c'est à dire qu'on ne garde qu'un seul pixel tous les N pixels
 */
class DecimatedImage : public Image {

private:

    /**
     * \~french \brief Image source à décimer
     * \~english \brief Source image, to decimate
     */
    Image* source_image;

    /**
     * \~french \brief Valeur de non-donnée
     * \details On a une valeur entière par canal. Tous les pixel de l'image composée seront initialisés avec cette valeur.
     * \~english \brief Nodata value
     */
    int* nodata;

    /**
     * \~french \brief Pas de la décimation dans le sens des X
     * \~english \brief Decimation's step, widthwise
     */
    int ratio_x; 
    /**
     * \~french \brief Pas de la décimation dans le sens des Y
     * \~english \brief Decimation's step, heightwise
     */
    int ratio_y;
    /**
     * \~french \brief Colonne du premier pixel de l'image source à être dans l'image décimée
     * \~english \brief First source pixel column to be in the decimated image
     */
    int source_offset_x;
    /**
     * \~french \brief Nombre de pixel à prendre dans l'image source pour composer l'image décimée
     * \details Peut être égal à zéro, auquel cas l'image source n'est pas lue et l'image décimée et pleine de #nodata
     * \~english \brief Pixel number to read in the source image, to build decimated image
     */
    int count_x;
    /**
     * \~french \brief Colonne du premier pixel de l'image décimée à provenir de l'image source
     * \~english \brief First decimated pixel column to come from the source image
     */
    int image_offset_x;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \details Lorsque l'on veut récupérer une ligne d'une image décimée, on ne garde que un pixel tous les #ratio_x de l'image source
     *
     * \param[in] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

    /** \~french
     * \brief Crée un objet DecimatedImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est privé afin de n'être appelé que par la méthode statique #create, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] image image source
     * \param[in] nd valeur de non-donnée
     ** \~english
     * \brief Create an DecimatedImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     * \param[in] image source image
     * \param[in] nd nodata value
     */
    DecimatedImage ( int width, int height, int channels, double resx, double resy, BoundingBox<double> bbox,
                            Image* image, int* nd );

public:

    /**
     * \~french
     * \brief Retourne le masque de l'image source
     * \return masque
     * \~english
     * \brief Return the mask of source image
     * \return mask
     */
    Image* get_mask () {
        return source_image->get_mask();
    }
    /**
     * \~french
     * \brief Retourne l'image source
     * \return image
     * \~english
     * \brief Return the source image
     * \return image
     */
    Image* get_source_image () {
        return source_image;
    }

    /**
     * \~french
     * \brief Retourne la valeur de non-donnée
     * \return Valeur de non-donnée
     * \~english
     * \brief Return the nodata value
     * \return nodata value
     */
    int* get_nodata_value() {
        return nodata;
    }

    int get_line ( uint8_t* buffer, int line );
    int get_line ( uint16_t* buffer, int line );
    int get_line ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression de toutes les images composant la DecimatedImage
     * \~english
     * \brief Default destructor
     */
    virtual ~DecimatedImage() {
        delete[] nodata;
        if ( ! is_mask ) {
            delete source_image;
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image composée
     ** \~english
     * \brief Compounded image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ DecimatedImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Number of picked source pixels : " << count_x;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Decimated image offset : " << image_offset_x;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Source image offset : " << source_offset_x;
        BOOST_LOG_TRIVIAL(info) <<  "\t- X wise ratio : " << ratio_x;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Y wise ratio : " << ratio_y;
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image composée et crée un objet DecimatedImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres. On vérifie la superposabilité des images sources.
     * \param[in] source_images source_images sources
     * \param[in] nodata valeur de non-donnée
     * \return un pointeur d'objet DecimatedImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate compounded image components and create an DecimatedImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters. We check if source images are superimpose.
     * \param[in] source_images source source_images
     * \param[in] nodata nodata value
     * \return a DecimatedImage object pointer, NULL if error
     */
    static DecimatedImage* create ( Image* image, BoundingBox<double> bb, double res_x, double res_y, int* nodata );
};

#endif
