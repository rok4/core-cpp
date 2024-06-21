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
 * \file LibjpegImage.cpp
 ** \~french
 * \brief Implémentation des classes LibjpegImage et LibjpegImageFactory
 * \details
 * \li LibjpegImage : gestion d'une image au format JPEG, en lecture, utilisant la librairie libpng
 * \li LibjpegImageFactory : usine de création d'objet LibjpegImage
 ** \~english
 * \brief Implement classes LibjpegImage and LibjpegImageFactory
 * \details
 * \li LibjpegImage : manage a JPEG format image, reading, using the library libpng
 * \li LibjpegImageFactory : factory to create LibjpegImage object
 */

#include "image/file/LibjpegImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"
#include "jpeglib.h"
#include <setjmp.h>

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibjpegImage* LibjpegImageFactory::createLibjpegImageToRead ( std::string filename, BoundingBox< double > bbox, double resx, double resy ) {

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    // Ouverture du fichier bianire en lecture
    FILE *file = fopen ( filename.c_str(), "rb" );
    if ( !file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open the file (to read) " << filename ;
        return NULL;
    }

    jpeg_stdio_src(&cinfo, file);

    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int channels = cinfo.num_components;

    Photometric::ePhotometric ph;
    if (channels == 3) {
        ph = Photometric::RGB;
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported JPEG sample number : " << channels;
        BOOST_LOG_TRIVIAL(error) <<  "\t for the image to read : " << filename ;
        return NULL;
    }

    /********************** CONTROLES **************************/
    
    if ( resx > 0 && resy > 0 ) {
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" ;
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    unsigned char** data = new unsigned char*[height];
    for (int i = 0; i < height; ++i)
    {
        data[i] = new unsigned char[width * channels];
    }
    unsigned char** ptr = data;

    while (cinfo.output_scanline < height) {
        jpeg_read_scanlines(&cinfo, ptr, 1);
        ptr ++;
    }


    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose ( file );

    /******************** CRÉATION DE L'OBJET ******************/
    
    return new LibjpegImage (
        width, height, resx, resy, channels, bbox, filename,
        SampleFormat::UINT8, ph, Compression::JPEG,
        data
    );
    
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibjpegImage::LibjpegImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric, Compression::eCompression compression,
    unsigned char** data ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sample_format, photometric, compression, ExtraSample::ALPHA_UNASSOC ),

    data(data) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibjpegImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];

    for (int x = 0;  x < width * channels; x++) {
        buffertmp[x] = data[line][x];
    }

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convertLine(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixel_size * width);
    }
    
    return width * get_channels();
}


int LibjpegImage::get_line ( uint8_t* buffer, int line ) {
    if ( sample_format == SampleFormat::UINT8 ) {
        return _getline ( buffer,line );
    } else if ( sample_format == SampleFormat::UINT16 ) { // uint16
        /* On ne convertit pas les entiers 16 bits en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer entier 16 bits sur le buffer entier, de même taille en octet (2 fois plus grand en "nombre de cases")*/
        uint16_t int16line[width * get_channels()];
        _getline ( int16line, line );
        memcpy ( buffer, int16line, width * get_pixel_size() );
        return width * get_pixel_size();
    } else if ( sample_format == SampleFormat::FLOAT32 ) { // float
        /* On ne convertit pas les nombres flottants en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer flottant sur le buffer entier, de même taille en octet (4 fois plus grand en "nombre de cases")*/
        float floatline[width * get_channels()];
        _getline ( floatline, line );
        memcpy ( buffer, floatline, width * get_pixel_size() );
        return width * get_pixel_size();
    }
    return 0;
}

int LibjpegImage::get_line ( uint16_t* buffer, int line ) {
    
    if ( sample_format == SampleFormat::UINT8 ) {
        // On veut la ligne en entier 16 bits mais l'image lue est sur 8 bits : on convertit
        uint8_t* buffer_t = new uint8_t[width * get_channels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * get_channels() );
        delete [] buffer_t;
        return width * get_channels();
    } else if ( sample_format == SampleFormat::UINT16 ) { // uint16
        return _getline ( buffer,line );        
    } else if ( sample_format == SampleFormat::FLOAT32 ) { // float
        /* On ne convertit pas les nombres flottants en entier sur 16 bits (aucun intérêt)
        * On va copier le buffer flottant sur le buffer entier 16 bits, de même taille en octet (2 fois plus grand en "nombre de cases")*/
        float floatline[width * channels];
        _getline ( floatline, line );
        memcpy ( buffer, floatline, width*pixel_size );
        return width*pixel_size;
    }
    return 0;
}

int LibjpegImage::get_line ( float* buffer, int line ) {
    if ( sample_format == SampleFormat::UINT8 ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width * get_channels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * get_channels() );
        delete [] buffer_t;
        return width * get_channels();
    } else if ( sample_format == SampleFormat::UINT16 ) { // uint16
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint16_t* buffer_t = new uint16_t[width * get_channels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * get_channels() );
        delete [] buffer_t;
        return width * get_channels();   
    } else if ( sample_format == SampleFormat::FLOAT32 ) { // float
        return _getline ( buffer, line );
    }
    return 0;
}



