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

//#include <stdint.h>// pour uint8_t
//#include <cstddef> // pour size_t
//#include <string>

#include "rok4/datasource/DataSource.h"
#include "rok4/image/Image.h"
#include "rok4/utils/Utils.h"

struct JpegDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size );
};

struct PngDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size );
};

struct LzwDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size );
};

struct DeflateDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size );
};

struct PackBitsDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size );
};

struct InvalidDecoder {
    static const uint8_t* decode ( DataSource* encoded_data, size_t &size ) {
        size = 0;
        return 0;
    }
};




/**
 * Classes Decoder
 */
template <class Decoder>
class DataSourceDecoder : public DataSource {
private:
    DataSource* encoded_data;
    const uint8_t* decoded_data;
    size_t decoded_size;
public:
    DataSourceDecoder ( DataSource* encoded_data ) : encoded_data ( encoded_data ), decoded_data ( 0 ), decoded_size ( 0 ) {}

    ~DataSourceDecoder() {
        if ( decoded_data )
            delete[] decoded_data;
        delete encoded_data;
    }

    const uint8_t* get_data ( size_t &size ) {
        if ( !decoded_data && encoded_data ) {
            decoded_data = Decoder::decode ( encoded_data, decoded_size );
            if ( !decoded_data ) {
                delete encoded_data;
                encoded_data = 0;
            }
        }
        size = decoded_size;
        return decoded_data;
    }

    bool release_data() {
        if ( encoded_data ) encoded_data->release_data();
        if ( decoded_data ) delete[] decoded_data;
        decoded_data = 0;
        return true;
    }

    std::string get_type() {
        return "image/bil";
    }
    int get_http_status() {
        return 200;
    }
    std::string get_encoding() {
        return "";
    }
    
    unsigned int get_length() {
        return decoded_size;
    }
};




class ImageDecoder : public Image {

private:

    DataSource* source_data;

    int source_width;
    int source_height;
    int margin_top;
    int margin_left;

    int channel_size; // type des images source : 1=uint8_t   2=uint16_t    4=float

    // La donnee brute (source) est de type uint8_t
    const uint8_t* raw_data;

    int get_data_line ( uint8_t* buffer, int line );

    int get_data_line ( uint16_t* buffer, int line );

    int get_data_line ( float* buffer, int line );

    template<typename T> inline int get_nodata_line ( T* buffer, int line ) {
        memset ( buffer, 0, width * channels * sizeof ( T ) );
        return width * channels;
    }

    // TODO : a deplacer dans le cpp (je n'y suis pas arrive a cause d un probleme de compilation lie au template)
    template<typename T>
    inline int _getline ( T* buffer, int line ) {

        if ( raw_data ) { // Est ce que l'on a de la donnee
            return get_data_line ( buffer, line );
            // TODO: libérer le source_data lorsque l'on lit la dernière ligne de l'image...
        } else if ( source_data ) { // Non alors on essaye de la l'initialiser depuis source_data
            size_t size;
            if ( raw_data = source_data->get_data ( size ) ) {
                return get_data_line ( buffer, line );
            } else {
                delete source_data;
                source_data = 0;
            }
        }
        //BOOST_LOG_TRIVIAL(debug) << "Decoding error, fill with black";
        return get_nodata_line ( buffer, line );
    }

public:

    ImageDecoder ( DataSource* source_data, int source_width, int source_height, int channels,
                   BoundingBox<double> bbox = BoundingBox<double> ( 0.,0.,0.,0. ),
                   int margin_left = 0, int margin_top = 0, int margin_right = 0, int margin_bottom = 0, int channel_size=1 ) :
        Image ( source_width - margin_left - margin_right, source_height - margin_top - margin_bottom, channels, bbox ),
        source_data ( source_data ),
        source_width ( source_width ),
        source_height ( source_height ),
        margin_top ( margin_top ),
        margin_left ( margin_left ),
        channel_size ( channel_size ),
        raw_data ( 0 ) {}

    /* Implémentation de l'interface Image */
    inline int get_line ( uint8_t* buffer, int line ) {
        return _getline ( buffer, line );
    }
    inline int get_line ( uint16_t* buffer, int line ) {
        return _getline ( buffer, line );
    }
    inline int get_line ( float* buffer, int line )   {
        return _getline ( buffer, line );
    }

    ~ImageDecoder() {
        if ( source_data ) {
            source_data->release_data();
            delete source_data;
        }
    }


};






