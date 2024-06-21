#include "datasource/TiffHeaderDataSource.h"
#include <string.h>

#include "datastream/TiffHeader.h"

TiffHeaderDataSource::TiffHeaderDataSource ( DataSource* source_data,
        Rok4Format::eFormat format, int channel,
        int width, int height, size_t tile_size ) :
    source_data ( source_data ),
    format ( format ), channel ( channel ),
    width ( width ), height ( height ) , tile_size ( tile_size ) {
    size_t header_size = TiffHeader::header_size ( channel );
    const uint8_t* tmp;
    data_size = header_size;
    if ( source_data ) {
        tmp = source_data->get_data ( tile_size );
        data_size+= tile_size;
    }

    data = new uint8_t[data_size];

    switch ( format ) {
    case Rok4Format::TIFF_RAW_UINT8:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_RAW_INT8_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_RAW_INT8_GRAY, header_size );
        } else if ( channel == 3 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_RAW_INT8_RGB" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_RAW_INT8_RGB, header_size );
        } else if ( channel == 4 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_RAW_INT8_RGBA" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_RAW_INT8_RGBA, header_size );
        }
        break;
    case Rok4Format::TIFF_LZW_UINT8:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_LZW_INT8_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_LZW_INT8_GRAY, header_size );
        } else if ( channel == 3 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_LZW_INT8_RGB" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_LZW_INT8_RGB, header_size );
        } else if ( channel == 4 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_LZW_INT8_RGBA" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_LZW_INT8_RGBA, header_size );
        }
        break;
    case Rok4Format::TIFF_ZIP_UINT8:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_ZIP_INT8_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_ZIP_INT8_GRAY, header_size );
        } else if ( channel == 3 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_ZIP_INT8_RGB" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_ZIP_INT8_RGB, header_size );
        } else if ( channel == 4 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_ZIP_INT8_RGBA" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_ZIP_INT8_RGBA, header_size );
        }
        break;
    case Rok4Format::TIFF_PKB_UINT8:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_PKB_INT8_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_PKB_INT8_GRAY, header_size );
        } else if ( channel == 3 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_PKB_INT8_RGB" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_PKB_INT8_RGB, header_size );
        } else if ( channel == 4 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_PKB_INT8_RGBA" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_PKB_INT8_RGBA, header_size );
        }
        break;

    case Rok4Format::TIFF_RAW_FLOAT32:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_RAW_FLOAT32_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_RAW_FLOAT32_GRAY, header_size );
        }
        break;
    case Rok4Format::TIFF_LZW_FLOAT32:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_LZW_FLOAT32_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_LZW_FLOAT32_GRAY, header_size );
        }
        break;
    case Rok4Format::TIFF_ZIP_FLOAT32:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_ZIP_FLOAT32_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_ZIP_FLOAT32_GRAY, header_size );
        }
        break;
    case Rok4Format::TIFF_PKB_FLOAT32:
        if ( channel == 1 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "TIFF_HEADER_PKB_FLOAT32_GRAY" ;
            memcpy ( data, TiffHeader::TIFF_HEADER_PKB_FLOAT32_GRAY, header_size );
        }
        break;
    }
    * ( ( uint32_t* ) ( data+18 ) )  = width;
    * ( ( uint32_t* ) ( data+30 ) )  = height;
    * ( ( uint32_t* ) ( data+102 ) ) = height;
    * ( ( uint32_t* ) ( data+114 ) ) = tile_size;

    if ( source_data ) {
        memcpy ( data+header_size, tmp, tile_size );
    }
}




const uint8_t* TiffHeaderDataSource::get_data ( size_t& size ) {
    size = data_size;
    return data;
}

TiffHeaderDataSource::~TiffHeaderDataSource() {
    if ( source_data ) {
        source_data->release_data();
        delete source_data;
    }
    if ( data )
        delete[] data;
}
