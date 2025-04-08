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

#include "datastream/TiffEncoder.h"

#include "datastream/TiffRawEncoder.h"
#include "datastream/TiffLZWEncoder.h"
#include "datastream/TiffDeflateEncoder.h"
#include "datastream/TiffPackBitsEncoder.h"


TiffEncoder::TiffEncoder(Image *image, int line, bool is_geotiff, int nodata): image(image), line(line), is_geotiff(is_geotiff), nodata(nodata) {
    tmp_buffer = NULL;
    tmp_buffer_pos = 0;
    tmp_buffer_size = 0;
    header = NULL;
    header_size = 0;
}

TiffEncoder::~TiffEncoder() {
    delete image;
    if ( tmp_buffer )
      delete[] tmp_buffer;
    if ( header )
      delete[] header;
}

size_t TiffEncoder::read(uint8_t* buffer, size_t size) {
    size_t offset = 0, dataToCopy=0;
    
    if ( !tmp_buffer ) {
        BOOST_LOG_TRIVIAL(debug) << "TiffEncoder : preparation du buffer d'image";
        prepare_buffer();
    }
    
    if ( !header ) {
        BOOST_LOG_TRIVIAL(debug) << "TiffEncoder : preparation de l'en-tete";
        prepare_header();
        if ( is_geotiff ){
            this->header = TiffHeader::insert_geo_tags(image, this->header, &(this->header_size), nodata );
        }
    }
    
    // Si pas assez de place pour le header, ne rien écrire.
    if ( size < header_size ) return 0;
      
    if ( line == -1 ) { // écrire le header tiff
	memcpy ( buffer, header, header_size );
	offset = header_size;
	line = 0;
    }

    if ( size - offset > 0 ) { // il reste de la place
	if ( tmp_buffer_pos <= tmp_buffer_size ) { // il reste de la donnée
	    dataToCopy = std::min ( size-offset, tmp_buffer_size - tmp_buffer_pos );
	    memcpy ( buffer+offset, tmp_buffer+tmp_buffer_pos, dataToCopy );
	    tmp_buffer_pos+=dataToCopy;
	    offset+=dataToCopy;
	}
    }

    return offset;
}

bool TiffEncoder::eof() {
    return ( tmp_buffer_pos>=tmp_buffer_size );
}

unsigned int TiffEncoder::get_length(){
    if ( !tmp_buffer ) {
        BOOST_LOG_TRIVIAL(debug) << "TiffEncoder : preparation du buffer d'image";
        prepare_buffer();
    }
    
    if ( !header ) {
        BOOST_LOG_TRIVIAL(debug) << "TiffEncoder : preparation de l'en-tete";
        prepare_header();
        if ( is_geotiff ){
            this->header = TiffHeader::insert_geo_tags(image, this->header, &(this->header_size), nodata );
        }
    }
    return header_size + tmp_buffer_size;
    
}

