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

#include <string.h>  // Pour memcpy
#include <zlib.h>

#include <algorithm>
#include <iostream>

#include "datastream/DataStream.h"
#include "datastream/TiffEncoder.h"
#include "datastream/TiffHeader.h"
#include "image/Image.h"

template <typename T>
class TiffDeflateEncoder : public TiffEncoder {

protected:
    T* buffer_line;
    z_stream zstream;

    bool encode() {
        int rawLine = 0;
        int error = 0;
        zstream.zalloc = Z_NULL;
        zstream.zfree = Z_NULL;
        zstream.opaque = Z_NULL;
        zstream.data_type = Z_BINARY;
        deflateInit(&zstream, 6);  // taux de compression zlib
        zstream.avail_in = 0;
        zstream.next_out = tmp_buffer;
        zstream.avail_out = tmp_buffer_size;

        while (rawLine >= 0 && rawLine < image->get_height() && zstream.avail_out > 0) {  // compresser les données dans des chunck idat
            if (zstream.avail_in == 0) {                                                 // si plus de donnée en entrée de la zlib, on lit une nouvelle ligne
                image->get_line(buffer_line, rawLine++);
                zstream.next_in = (uint8_t*)(buffer_line);
                zstream.avail_in = image->get_width() * image->get_channels() * sizeof(T);
            }
            error = deflate(&zstream, Z_NO_FLUSH);
            switch (error) {
                case Z_OK:
                    break;
                case Z_MEM_ERROR:
                    BOOST_LOG_TRIVIAL(debug) << "MEM_ERROR";
                    deflateEnd(&zstream);
                    return false;  // return 0 en cas d'erreur.
                case Z_STREAM_ERROR:
                    BOOST_LOG_TRIVIAL(debug) << "STREAM_ERROR";
                    deflateEnd(&zstream);
                    return false;  // return 0 en cas d'erreur.
                case Z_VERSION_ERROR:
                    BOOST_LOG_TRIVIAL(debug) << "VERSION_ERROR";
                    deflateEnd(&zstream);
                    return false;  // return 0 en cas d'erreur.
                default:
                    BOOST_LOG_TRIVIAL(debug) << "OTHER_ERROR";
                    deflateEnd(&zstream);
                    return false;  // return 0 en cas d'erreur.
            }
        }

        if (rawLine == image->get_height() && zstream.avail_out > 6) {  // plus d'entrée : il faut finaliser la compression
            int r = deflate(&zstream, Z_FINISH);
            if (r == Z_STREAM_END)
                rawLine++;  // on indique que l'on a compressé fini en passant rawLine ) height+1
            else if (r != Z_OK) {
                deflateEnd(&zstream);
                return false;  // une erreur
            }
        }

        if (deflateEnd(&zstream) != Z_OK) return false;

        uint32_t length = zstream.total_out;  // taille des données écritres
        tmp_buffer_size = length;
        return true;
    }

    virtual void prepare_header() {
        BOOST_LOG_TRIVIAL(debug) << "TiffDeflateEncoder : preparation de l'en-tete";
        header_size = TiffHeader::header_size(image->get_channels());
        header = new uint8_t[header_size];
        if (image->get_channels() == 1)
            if (sizeof(T) == sizeof(float)) {
                memcpy(header, TiffHeader::TIFF_HEADER_ZIP_FLOAT32_GRAY, header_size);
            } else {
                memcpy(header, TiffHeader::TIFF_HEADER_ZIP_INT8_GRAY, header_size);
            }
        else if (image->get_channels() == 3)
            memcpy(header, TiffHeader::TIFF_HEADER_ZIP_INT8_RGB, header_size);
        else if (image->get_channels() == 4)
            memcpy(header, TiffHeader::TIFF_HEADER_ZIP_INT8_RGBA, header_size);
        *((uint32_t*)(header + 18)) = image->get_width();
        *((uint32_t*)(header + 30)) = image->get_height();
        *((uint32_t*)(header + 102)) = image->get_height();
        *((uint32_t*)(header + 114)) = tmp_buffer_size;
    }

    virtual void prepare_buffer() {
        BOOST_LOG_TRIVIAL(debug) << "TiffDeflateEncoder : preparation du buffer d'image";
        tmp_buffer_size = image->get_width() * image->get_channels() * image->get_height() * 2 * sizeof(T);
        tmp_buffer = new uint8_t[tmp_buffer_size];
        while (!encode()) {
            tmp_buffer_size *= 2;
            delete[] tmp_buffer;
            tmp_buffer = new uint8_t[tmp_buffer_size];
        }
    }

public:
    TiffDeflateEncoder(Image* image, bool is_geotiff = false) : TiffEncoder(image, -1, is_geotiff) {
        //         zstream.zalloc = Z_NULL;
        //         zstream.zfree = Z_NULL;
        //         zstream.opaque = Z_NULL;
        //         zstream.data_type = Z_BINARY;
        //         deflateInit ( &zstream, 6 ); // taux de compression zlib
        //         zstream.avail_in = 0;
        buffer_line = new T[image->get_width() * image->get_channels()];
    }
    ~TiffDeflateEncoder() {
        if (buffer_line) delete[] buffer_line;
        //         deflateEnd ( &zstream );
    }

    std::string get_encoding() {
        return "deflate";
    }
};


