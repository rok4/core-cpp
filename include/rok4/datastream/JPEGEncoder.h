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

#pragma once

#include "rok4/datastream/DataStream.h"
#include "rok4/image/Image.h"
#include <jpeglib.h>

/** D */
class JPEGEncoder : public DataStream {
    
private:

    Image *image;

    int status;
    int buffer_limit;
    uint8_t *buffer_line;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    static void init_destination ( jpeg_compress_struct *cinfo ) {
        return;
    }
    static boolean empty_output_buffer ( jpeg_compress_struct *cinfo ) {
        return false;
    }
    static void term_destination ( jpeg_compress_struct *cinfo ) {
        return;
    }

public:

    /** D */
    JPEGEncoder ( Image* image, int quality = 75 );

    /** D */
    ~JPEGEncoder();

    /**
     * Lecture du flux JPEG
     */
    size_t read ( uint8_t *buffer, size_t size );

    bool eof() {
        return ( cinfo.next_scanline >= cinfo.image_height );
    }

    std::string get_type() {
        return "image/jpeg";
    }

    int get_http_status() {
        return 200;
    }
    
    std::string get_encoding() {
        return "";
    }
    
    unsigned int get_length() {
        return 0;
    }
};




