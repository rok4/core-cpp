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

#include "image/PaletteImage.h"

#include <boost/log/trivial.hpp>

int PaletteImage::get_line ( float* buffer, int line ) {
    if ( source_image->get_channels() == 1 && ! palette->is_empty() ) {
        return _getline ( buffer, line );
    } else {
        return source_image->get_line ( buffer, line );
    }
}

int PaletteImage::get_line ( uint16_t* buffer, int line ) {
    if ( source_image->get_channels() == 1 && ! palette->is_empty() ) {
        return _getline ( buffer, line );
    } else {
        return source_image->get_line ( buffer, line );
    }
}

int PaletteImage::get_line ( uint8_t* buffer, int line ) {
    if ( source_image->get_channels() == 1 && ! palette->is_empty() ) {
        return _getline ( buffer, line );
    } else {
        return source_image->get_line ( buffer, line );
    }
}

PaletteImage::PaletteImage ( Image* image, Palette* palette ) : Image ( image->get_width(), image->get_height(), 1, image->get_bbox() ), source_image ( image ), palette ( palette ) {
    // Il n'y aura application de la palette et modification des canaux que si
    // - la palette n'est pas vide
    // - l'image source est sur un canal
    if ( source_image->get_channels() == 1 && ! this->palette->is_empty() ) {
        if (this->palette->is_no_alpha()) {
            channels = 3;
        } else {
            channels = 4;
        }
    } else {
        channels = image->get_channels();
    }
}

PaletteImage::~PaletteImage() {
    delete source_image;
}

template<typename T>
int PaletteImage::_getline ( T* buffer, int line ) {
    float* source = new float[source_image->get_width() * source_image->get_channels()];
    source_image->get_line ( source, line );
    switch ( channels ) {
    case 4:
        for (int i = 0; i < source_image->get_width() ; i++ ) {
            Colour iColour = palette->get_colour ( * ( source+i ) );
            * ( buffer+i*4 ) = (T) iColour.r;
            * ( buffer+i*4+1 ) = (T) iColour.g;
            * ( buffer+i*4+2 ) = (T) iColour.b;
            * ( buffer+i*4+3 ) = (T) iColour.a;
        }
        break;
        
    case 3:
        for (int i = 0; i < source_image->get_width() ; i++ ) {
            Colour iColour = palette->get_colour ( * ( source+i ) );
            * ( buffer+i*3 ) = (T) iColour.r;
            * ( buffer+i*3+1 ) = (T) iColour.g;
            * ( buffer+i*3+2 ) = (T) iColour.b;
        }
        break;
    }

    

    delete[] source;
    return width * sizeof ( T ) * channels;
}
