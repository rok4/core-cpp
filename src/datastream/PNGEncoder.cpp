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

#include <iostream>

#include "datastream/PNGEncoder.h"
#include "byteswap.h"
#include <boost/log/trivial.hpp>
#include <string.h> // Pour memcpy


// IEND chunck
static const uint8_t IEND[12] = {
    0, 0, 0, 0, 'I', 'E', 'N', 'D',    // 8  | taille et type du chunck IHDR
    0xae, 0x42, 0x60, 0x82
};           // crc32

static const uint8_t PNG_HEADER_GRAY[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    0,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_RGB[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    2,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_RGBA[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    6,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_PALETTE[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    3,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};


//For PNG compression method 0, the zlib compression method/flags code shall specify method code 8 (deflate compression) and an LZ77 window size of not more than 32768 bytes. The zlib compression method number is not the same as the PNG compression method number in the IHDR chunk (see 11.2.2 IHDR Image header). The additional flags shall not specify a preset dictionary.
//

void PNGEncoder::addCRC ( uint8_t *buffer, uint32_t length ) {
    * ( ( uint32_t* ) buffer ) = bswap_32 ( length );
    uint32_t crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + 4, length + 4 );
    * ( ( uint32_t* ) ( buffer+8+length ) ) = bswap_32 ( crc );
}

size_t PNGEncoder::write_IHDR ( uint8_t *buffer, size_t size ) {
    size_t s = 0;

    // cf: http://www.w3.org/TR/PNG/#11IHDR
    if ( image->get_channels() == 0 && (palette == NULL || palette->is_empty())) {
        // On est sur du 1 canal gris
        if ( sizeof ( PNG_HEADER_GRAY ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_GRAY, sizeof ( PNG_HEADER_GRAY ) ); 
        s = sizeof ( PNG_HEADER_GRAY );
    } else if ( image->get_channels() == 0) {
        // On est sur du 1 canal palette
        if ( sizeof ( PNG_HEADER_PALETTE ) + palette->get_png_palette_size() > size ) return 0;
        memcpy ( buffer, PNG_HEADER_PALETTE, sizeof ( PNG_HEADER_PALETTE ) );
        memcpy ( buffer + sizeof ( PNG_HEADER_PALETTE ), palette->get_png_palette(), palette->get_png_palette_size() );
        s = sizeof ( PNG_HEADER_PALETTE ) + palette->get_png_palette_size();
    } else if ( image->get_channels() == 3 ) {
        // On est sur du 3 canaux RGB
        if ( sizeof ( PNG_HEADER_RGB ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_RGB, sizeof ( PNG_HEADER_RGB ) );
        s = sizeof ( PNG_HEADER_RGB );
    } else if ( image->get_channels() == 4 ) {
        // On est sur du 4 canaux RGBA
        if ( sizeof ( PNG_HEADER_RGBA ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_RGBA, sizeof ( PNG_HEADER_RGBA ) );
        s = sizeof ( PNG_HEADER_RGBA );
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled bands count for PNG encoder : " << image->get_channels() ;
        return 0;
    }

    // On reseigne la taille
    * ( ( uint32_t* ) ( buffer+16 ) ) = bswap_32 ( image->get_width() );
    * ( ( uint32_t* ) ( buffer+20 ) ) = bswap_32 ( image->get_height() );

    addCRC ( buffer+8, 13 ); // on signe le chunck avca un CRC32
    line++;

    return s;
}

size_t PNGEncoder::write_IEND ( uint8_t *buffer, size_t size ) {
    if ( sizeof ( IEND ) > size ) return 0;
    memcpy ( buffer, IEND, sizeof ( IEND ) );
    line++;
    return sizeof ( IEND );
}

size_t PNGEncoder::write_IDAT ( uint8_t *buffer, size_t size ) {
    if ( size <= 12 ) return 0;
    buffer[4] = 'I';
    buffer[5] = 'D';
    buffer[6] = 'A';
    buffer[7] = 'T';

    zstream.next_out  = buffer + 8; // laisser 8 octets au debut pour le header du chunck
    zstream.avail_out = size - 12;  // et 4 octets à la fin pour crc32

    while ( line >= 0 && line < image->get_height() && zstream.avail_out > 0 ) { // compresser les données dans des chunck idat
        if ( zstream.avail_in == 0 ) {                                    // si plus de donnée en entrée de la zlib, on lit une nouvelle ligne
            image->get_line ( buffer_line+1, line++ );
            zstream.next_in  = ( ( uint8_t* ) ( buffer_line+1 ) ) - 1;
            zstream.avail_in = image->get_width() * image->get_channels() + 1;
        }
        if ( deflate ( &zstream, Z_NO_FLUSH ) != Z_OK ) return 0;         // return 0 en cas d'erreur.
    }

    if ( line == image->get_height() && zstream.avail_out > 6 ) { // plus d'entrée : il faut finaliser la compression
        int r = deflate ( &zstream, Z_FINISH );
        if ( r == Z_STREAM_END ) line++;                   // on indique que l'on a compressé fini en passant line ) height+1
        else if ( r != Z_OK ) return 0;                    // une erreur
    }
    uint32_t length = size - zstream.avail_out;   // taille des données écritres
    addCRC ( buffer, length - 12 );               // signature du chunck
    return length;
}


size_t PNGEncoder::read ( uint8_t *buffer, size_t size ) {
    size_t pos = 0;
    uint8_t colortype=2;
    // On traite 2 cas : 'Greyscale' et 'Truecolor'
    // cf: http://www.w3.org/TR/PNG/#11IHDR

    if ( line == -1 ) pos += write_IHDR ( buffer, size );
    if ( line >= 0 && line <= image->get_height() ) pos += write_IDAT ( buffer + pos, size - pos );
    if ( line == image->get_height() +1 ) pos += write_IEND ( buffer + pos, size - pos );
    return pos;
}

bool PNGEncoder::eof() {
    return ( line > image->get_height() +1 );
}

PNGEncoder::PNGEncoder ( Image* image,Palette* palette ) : image ( image ), line ( -1 ), palette ( palette ) , stubpalette ( NULL ) {
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.data_type = Z_BINARY;
    deflateInit ( &zstream, 5 ); // taux de compression zlib
    zstream.avail_in = 0;
    buffer_line = new uint8_t[image->get_width() * image->get_channels() + 1]; // On rajoute une valeur en plus pour l'index de debut de ligne png qui sera toujours 0 dans notre cas. TODO : essayer d'aligner en memoire pour des get_line plus efficace
    buffer_line[0] = 0;
    if ( ! palette ) {
        stubpalette = new Palette();
        palette = stubpalette;
    }
}

PNGEncoder::~PNGEncoder() {
    deflateEnd ( &zstream );
    if ( buffer_line ) delete[] buffer_line;
    delete image;
    if ( stubpalette )
        delete stubpalette;
}

