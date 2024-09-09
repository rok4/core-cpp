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

#include <algorithm>
#include <iostream>

#include "compressors/LzwCompressor.h"
#include "datastream/DataStream.h"
#include "datastream/TiffEncoder.h"
#include "datastream/TiffHeader.h"
#include "image/Image.h"

template <typename T>
class TiffLZWEncoder : public TiffEncoder {
   protected:
    size_t raw_buffer_size;
    T* raw_buffer;

    virtual void prepare_header() {
        BOOST_LOG_TRIVIAL(debug) << "TiffLZWEncoder : preparation de l'en-tete";
        header_size = TiffHeader::header_size(image->get_channels());
        header = new uint8_t[header_size];
        if (image->get_channels() == 1)
            if (sizeof(T) == sizeof(float)) {
                memcpy(header, TiffHeader::TIFF_HEADER_LZW_FLOAT32_GRAY, header_size);
            } else {
                memcpy(header, TiffHeader::TIFF_HEADER_LZW_INT8_GRAY, header_size);
            }
        else if (image->get_channels() == 3)
            memcpy(header, TiffHeader::TIFF_HEADER_LZW_INT8_RGB, header_size);
        else if (image->get_channels() == 4)
            memcpy(header, TiffHeader::TIFF_HEADER_LZW_INT8_RGBA, header_size);
        *((uint32_t*)(header + 18)) = image->get_width();
        *((uint32_t*)(header + 30)) = image->get_height();
        *((uint32_t*)(header + 102)) = image->get_height();
        *((uint32_t*)(header + 114)) = tmp_buffer_size;
    }

    virtual void prepare_buffer() {
        BOOST_LOG_TRIVIAL(debug) << "TiffLZWEncoder : preparation du buffer d'image";
        int linesize = image->get_width() * image->get_channels();
        raw_buffer = new T[image->get_height() * image->get_width() * image->get_channels()];
        raw_buffer_size = 0;
        int lRead = 0;
        for (; lRead < image->get_height(); lRead++) {
            image->get_line(raw_buffer + raw_buffer_size, lRead);
            raw_buffer_size += linesize;
        }
        raw_buffer_size *= sizeof(T);
        LzwCompressor encoder;
        tmp_buffer = encoder.encode((uint8_t*)raw_buffer, raw_buffer_size, tmp_buffer_size);
        delete[] raw_buffer;
        raw_buffer = NULL;
    }

   public:
    TiffLZWEncoder(Image* image, bool is_geotiff = false) : TiffEncoder(image, -1, is_geotiff), raw_buffer_size(0), raw_buffer(NULL) {}

    ~TiffLZWEncoder() {
    }
};


