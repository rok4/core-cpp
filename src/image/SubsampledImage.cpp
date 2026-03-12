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
 * \file SubsampledImage.cpp
 ** \~french
 * \brief Implémentation des classes SubsampledImage, DecimatedMask
 * \details
 * \li SubsampledImage : image calculée à  d'images compatibles, superposables
 * \li DecimatedMask : masque composé, associé à une image composée
 ** \~english
 * \brief Implement classes SubsampledImage, DecimatedMask
 * \details
 * \li SubsampledImage : image compounded with superimpose images
 * \li DecimatedMask : compounded mask, associated with a compounded image
 */

#include "image/SubsampledImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"


/********************************************** SubsampledImage ************************************************/

template <typename T>
int SubsampledImage::_getline ( T* buffer, int line ) {

    // On a besoin de récupérer ratio_y lignes dans l'image source
    T* source_image_lines = new T[ratio_y * source_image->get_width() * source_image->get_channels()];
    uint8_t* source_mask_lines = NULL;
    
    for ( int i = 0; i < ratio_y; i++ ) {
        if (source_image->get_line ( source_image_lines + i * source_image->get_width() * source_image->get_channels(), line * ratio_y + i ) == 0) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot read line " << line * ratio_y + i << "from source to process SubsampledImage's line " << line ;
            return 0;
        }
    }

    // On récupère les lignes de masques si il est présent
    bool mask = false;
    if (source_image->get_mask() != NULL) {
        mask = true;
        source_mask_lines = new uint8_t[ratio_y * source_image->get_width()];
        for ( int i = 0; i < ratio_y; i++ ) {
            if (source_image->get_mask()->get_line ( source_mask_lines + i * source_image->get_width(), line * ratio_y + i ) == 0) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read mask line " << line * ratio_y + i << "from source to process SubsampledImage's line " << line ;
                return 0;
            }
        }
    }

    for (int pixel = 0; pixel < width; pixel++) {

        int data_count;

        for (int band = 0; band < channels; band++) {
            data_count = 0;
            float value = 0;

            for (int x = 0; x < ratio_x; x++) {
                for (int y = 0; y < ratio_y; y++) {

                    if (mask && source_mask_lines[source_image->get_width() * y + pixel * ratio_x + x] == 0) {
                        // ce n'est pas un pixel de donnée, on passe
                        continue;
                    }

                    data_count++;
                    value += source_image_lines[source_image->get_width() * channels * y + pixel * ratio_x * channels + x * channels + band];
                }   
            }

            buffer[pixel * channels + band] = (T) (value / data_count);
        }

        if (data_count == 0) {
            // On avait aucun pixel de donnée, on va mettre la valeur du premier pixel de la zone, considéré comme le nodata de l'image en entrée
            for (int band = 0; band < channels; band++) {
                buffer[pixel * channels + band] = source_image_lines[pixel * ratio_x * channels + band];
            }
        }
    }    
    
    delete [] source_image_lines;
    if (source_mask_lines != 0) {
        delete [] source_mask_lines;
    }
    
    return width * channels;
}


/* Implementation de get_line pour les uint8_t */
int SubsampledImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int SubsampledImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int SubsampledImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

SubsampledImage::SubsampledImage ( Image* image, int ratio_x, int ratio_y ) :
    Image ( image->get_width() / ratio_x, image->get_height() / ratio_y, image->get_channels(), image->get_bbox() ),
    source_image ( image ), ratio_x (ratio_x), ratio_y (ratio_y) {
    
}

SubsampledImage* SubsampledImage::create ( Image* image, int ratio_x, int ratio_y ) {

    if ( image == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "No source image to define subsampled image" ;
        return NULL;
    }
    
    // On vérifie que les facteur de sous échantillonnage

    if (image->get_width() % ratio_x != 0) {
        BOOST_LOG_TRIVIAL(error) <<  "Width have to be a multiple of widthwise subsampling factor" ;
        return NULL;        
    }
    
    if (image->get_height() % ratio_y != 0) {
        BOOST_LOG_TRIVIAL(error) <<  "Height have to be a multiple of heightwise subsampling factor" ;
        return NULL;        
    }
        
    SubsampledImage* sub = new SubsampledImage ( image, ratio_x, ratio_y);

    return sub;
}

