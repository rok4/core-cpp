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
 * \file Format.cpp
 ** \~french
 * \brief Implémentation des namespaces Compression, SampleFormat, Photometric, ExtraSample et Format
 * \details
 * \li SampleFormat : gère les types de canaux acceptés par les classes d'Image
 * \li Compression : énumère et manipule les différentes compressions
 * \li Format : énumère et manipule les différentes format d'image
 * \li Photometric : énumère et manipule les différentes photométries
 * \li ExtraSample : énumère et manipule les différents type de canal supplémentaire
 ** \~english
 * \brief Implement the namespaces Compression, SampleFormat, Photometric, ExtraSample et Format
 * \details
 * \li SampleFormat : managed sample type accepted by Image classes
 * \li Compression : enumerate and managed different compressions
 * \li Format : enumerate and managed different formats
 * \li Photometric : enumerate and managed different photometrics
 * \li ExtraSample : enumerate and managed different extra sample types
 */

#include "enums/Format.h"
#include <string.h>
#include <ctype.h>
#include <algorithm>

namespace Compression {

const char *compression_name[] = {
    "UNKNOWN",
    "NONE",
    "DEFLATE",
    "JPEG",
    "JPEG90",
    "PNG",
    "LZW",
    "PACKBITS",
    "JPEG2000"
};

eCompression from_string ( std::string strComp ) {
    int i;
    for ( i = compression_size; i ; --i ) {
        if ( strComp.compare ( compression_name[i] ) == 0 )
            break;
    }
    return static_cast<eCompression> ( i );
}

std::string to_string ( eCompression comp ) {
    return std::string ( compression_name[comp] );
}

}

namespace Photometric {

const char *photometric_name[] = {
    "UNKNOWN",
    "GRAY",
    "RGB",
    "PALETTE",
    "YCBCR",
    "MASK"
};

ePhotometric from_string ( std::string strPh ) {  

    int i;
    std::transform(strPh.begin(), strPh.end(), strPh.begin(), toupper);

    for ( i = photometric_size; i ; --i ) {
        if ( strPh.compare ( photometric_name[i] ) == 0 )
            break;
    }
    return static_cast<ePhotometric> ( i );
}

std::string to_string ( ePhotometric ph ) {
    return std::string ( photometric_name[ph] );
}

}

namespace ExtraSample {

const char *extraSample_name[] = {
    "UNKNOWN",
    "NONE",
    "ASSOCIATED ALPHA",
    "UNASSOCIATED ALPHA"
};

eExtraSample from_string ( std::string strPh ) {
    int i;
    for ( i = extraSample_size; i ; --i ) {
        if ( strPh.compare ( extraSample_name[i] ) == 0 )
            break;
    }
    return static_cast<eExtraSample> ( i );
}

std::string to_string ( eExtraSample es ) {
    return std::string ( extraSample_name[es] );
}

}

namespace SampleFormat {

const char *sampleformat_name[] = {
    "UNKNOWN",

    "UINT8",
    "UINT16",
    "FLOAT32"
};


const int esampleformat_bitspersample[] = {
    -1,

    8,
    16,
    32
};

eSampleFormat from_string ( std::string strSF ) {
    int i;
    for ( i = sampleformat_size; i ; --i ) {
        if ( strSF.compare ( sampleformat_name[i] ) == 0 )
            break;
    }
    return static_cast<eSampleFormat> ( i );
}

std::string to_string ( eSampleFormat sf ) {
    return std::string ( sampleformat_name[sf] );
}

int get_bits_per_sample ( eSampleFormat sf ) {
    return esampleformat_bitspersample[sf];
}

}

namespace Rok4Format {

const char *eformat_name[] = {
    "UNKNOWN",

    "TIFF_RAW_UINT8",
    "TIFF_JPG_UINT8",
    "TIFF_JPG90_UINT8",
    "TIFF_PNG_UINT8",
    "TIFF_LZW_UINT8",
    "TIFF_ZIP_UINT8",
    "TIFF_PKB_UINT8",

    "TIFF_RAW_FLOAT32",
    "TIFF_LZW_FLOAT32",
    "TIFF_ZIP_FLOAT32",
    "TIFF_PKB_FLOAT32",

    "TIFF_PBF_MVT"
};

const bool eformat_israster[] = {
    false,

    true,
    true,
    true,
    true,
    true,
    true,
    true,

    true,
    true,
    true,
    true,

    false
};

const int eformat_channelsize[] = {
    -1,

    1,
    1,
    1,
    1,
    1,
    1,
    1,
    
    4,
    4,
    4,
    4,

    -1
};

const char *eformat_mime[] = {
    "UNKNOWN",

    "image/tiff",
    "image/jpeg",
    "image/jpeg",
    "image/png",
    "image/tiff",
    "image/tiff",
    "image/tiff",

    "image/x-bil;bits=32",
    "image/tiff",
    "image/x-bil;bits=32",
    "image/tiff",

    "application/x-protobuf"
};

const char *eformat_tiles[] = {
    "UNKNOWN",

    "tiff",
    "jpg",
    "jpg",
    "png",
    "tiff",
    "tiff",
    "tiff",

    "tiff",
    "tiff",
    "tiff",
    "tiff",

    "mvt"
};

const char *eformat_extension[] = {
    "UNKNOWN",

    "tif",
    "jpeg",
    "jpeg",
    "png",
    "tif",
    "tif",
    "tif",

    "bil",
    "tif",
    "zbil",
    "tif",

    "pbf"
};

const char *eformat_encoding[] = {
    "",

    "",
    "",
    "",
    "",
    "",
    "",
    "",

    "",
    "",
    "deflate",
    "",

    ""
};


const Compression::eCompression eformat_compression[] = {
    Compression::UNKNOWN,

    Compression::NONE,
    Compression::JPEG,
    Compression::JPEG90,
    Compression::PNG,
    Compression::LZW,
    Compression::DEFLATE,
    Compression::PACKBITS,

    Compression::NONE,
    Compression::LZW,
    Compression::DEFLATE,
    Compression::PACKBITS,

    Compression::UNKNOWN
};


const SampleFormat::eSampleFormat eformat_sampleformat[] = {
    SampleFormat::UNKNOWN,

    SampleFormat::UINT8,
    SampleFormat::UINT8,
    SampleFormat::UINT8,
    SampleFormat::UINT8,
    SampleFormat::UINT8,
    SampleFormat::UINT8,
    SampleFormat::UINT8,

    SampleFormat::FLOAT32,
    SampleFormat::FLOAT32,
    SampleFormat::FLOAT32,
    SampleFormat::FLOAT32,

    SampleFormat::UNKNOWN
};

const int eformat_bitspersample[] = {
    -1,

    8,
    8,
    8,
    8,
    8,
    8,
    8,

    32,
    32,
    32,
    32,

    -1
};

eFormat from_string ( std::string strFormat ) {
    int i;
    for ( i=eformat_size; i ; --i ) {
        if ( strFormat.compare ( eformat_name[i] ) ==0 )
            break;
    }
    return static_cast<eFormat> ( i );
}

std::string to_string ( eFormat format ) {
    return std::string ( eformat_name[format] );
}

bool is_raster ( eFormat format ) {
    return eformat_israster[format];
}

Compression::eCompression get_compression ( eFormat format ) {
    return eformat_compression[format];
}

SampleFormat::eSampleFormat get_sample_format ( eFormat format ) {
    return eformat_sampleformat[format];
}

int get_bits_per_sample ( eFormat format ) {
    return eformat_bitspersample[format];
}

std::string to_mime_type ( eFormat format ) {
    return std::string ( eformat_mime[format] );
}

std::string to_tiles_format ( eFormat format ) {
    return std::string ( eformat_tiles[format] );
}

std::string to_extension ( eFormat format ) {
    return std::string ( eformat_extension[format] );
}

std::string to_encoding ( eFormat format ) {
    return std::string ( eformat_encoding[format] );
}

}
