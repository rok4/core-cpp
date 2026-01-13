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
 * \file SubsampledImage.h
 ** \~french
 * \brief Définition des classes SubsampledImage
 * \details
 * \li SubsampledImage : image résultant du sous échantillonnage d'une image source
 ** \~english
 * \brief Define classes SubsampledImage
 * \details
 * \li SubsampledImage : image built by source image's sub sampling
 */

#pragma once

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
class SubsampledImage : public Image {

private:

    /**
     * \~french \brief Image source à décimer
     * \~english \brief Source image, to decimate
     */
    Image* source_image;

    /**
     * \~french \brief Facteur de sous échantillonnage dans le sens des X
     * \~english \brief Subsampling's factor, widthwise
     */
    int ratio_x; 
    /**
     * \~french \brief Facteur de sous échantillonnage dans le sens des Y
     * \~english \brief Subsampling's factor, heightwise
     */
    int ratio_y;

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
     * \brief Crée un objet SubsampledImage à partir de tous ses éléments constitutifs
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
     * \brief Create an SubsampledImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     * \param[in] image source image
     * \param[in] nd nodata value
     */
    SubsampledImage ( Image* image, int ratio_x, int ratio_y);

public:

    int get_line ( uint8_t* buffer, int line );
    int get_line ( uint16_t* buffer, int line );
    int get_line ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression de l'image source de la SubsampledImage
     * \~english
     * \brief Default destructor
     */
    virtual ~SubsampledImage() {
        if (! is_mask) {
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
        BOOST_LOG_TRIVIAL(info) <<  "------ SubsampledImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- X wise ratio : " << ratio_x;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Y wise ratio : " << ratio_y;
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

    /** \~french
     * \brief Teste et calcule les caractéristiques de l'image sous échantillonnée et crée un objet SubsampledImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres. On vérifie que les dimensions sont divisibles par les facteurs.
     * \param[in] image image source
     * \param[in] ratio_x facteur de sous échantillonnage en X
     * \param[in] ratio_y facteur de sous échantillonnage en Y
     * \return un pointeur d'objet SubsampledImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate compounded image components and create an SubsampledImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters. We check if dimensions are multiple of factors.
     * \param[in] image source image
     * \param[in] ratio_x Widthwise subsampling factor
     * \param[in] ratio_y Heightwise subsampling factor
     * \return a SubsampledImage object pointer, NULL if error
     */
    static SubsampledImage* create ( Image* image, int ratio_x, int ratio_y );
};


