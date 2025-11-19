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
 * \file TerrainrgbImage.cpp
 * \~french
 * \brief Implémentation de la classe TerrainrgbImage permettant l'application du terrainrgb à une image.
 * \~english
 * \brief Implement the TerrainrgbImage Class handling computing of Terrainrgb style to an image.
 */
#include "image/StyledImage.h"
#include <boost/log/trivial.hpp>

int StyledImage::get_line(float *buffer, int line) {
    return styled_image->get_line(buffer, line);
}

int StyledImage::get_line(uint16_t *buffer, int line) {
    return styled_image->get_line(buffer, line);
}

int StyledImage::get_line(uint8_t *buffer, int line) {
    return styled_image->get_line(buffer, line);
}

StyledImage::StyledImage(Image *input_images, Style *style) :Image(input_images->get_width(), input_images->get_height(), 1, input_images->get_bbox()) {
    Image* mid_image = NULL;
    styled_image = input_images;

    if (style->estompage_defined()) {
        mid_image = new EstompageImage (input_images, style->get_estompage());
    }
    else if (style->pente_defined()) {
        mid_image = new PenteImage (input_images, style->get_pente());
    }
    else if (style->aspect_defined()) {
        mid_image = new AspectImage (input_images, style->get_aspect()) ;           
    }
    else if (style->terrainrgb_defined()) {
        mid_image = new TerrainrgbImage (input_images, style->get_terrainrgb()) ;          
    }
    if (style->palette_defined()){
        if ( styled_image->get_channels() == 1 && ! ( style->get_palette()->is_empty() ) ) {
            if (mid_image != NULL) {
                styled_image = new PaletteImage ( mid_image , style->get_palette() );
            } else {
                styled_image = new PaletteImage ( input_images , style->get_palette() );
            }
        } 
    }else {
        if (mid_image != NULL) {
            styled_image = mid_image;
        }
    }
}

StyledImage::~StyledImage() {
    delete styled_image;
}
