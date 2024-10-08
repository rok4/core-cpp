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
 * \file LibpngImage.cpp
 ** \~french
 * \brief Implémentation des classes LibpngImage
 * \details
 * \li LibpngImage : gestion d'une image au format PNG, en lecture, utilisant la librairie libpng
 ** \~english
 * \brief Implement classes LibpngImage
 * \details
 * \li LibpngImage : manage a PNG format image, reading, using the library libpng
 */

#include "image/file/LibpngImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"
#include "datastream/DataStream.h"

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */

static Photometric::ePhotometric to_rok4_photometric ( png_byte ph ) {
    switch ( ph ) {
    case PNG_COLOR_TYPE_GRAY :
        return Photometric::GRAY;
    case PNG_COLOR_TYPE_GRAY_ALPHA :
        return Photometric::GRAY;
    case PNG_COLOR_TYPE_PALETTE :
        return Photometric::RGB;
    case PNG_COLOR_TYPE_RGB :
        return Photometric::RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA :
        return Photometric::RGB;
    default :
        return Photometric::UNKNOWN;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibpngImage* LibpngImage::create_to_read ( std::string filename, BoundingBox< double > bbox, double resx, double resy ) {


    png_structp pngStruct;
    png_infop pngInfo;
    png_bytep * pngData;

    png_byte header[8];    // 8 is the maximum size that can be checked

    // Ouverture du fichier bianire en lecture
    FILE *file = fopen ( filename.c_str(), "rb" );
    if ( !file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open the file (to read) " << filename ;
        return NULL;
    }
    
    // Vérification de l'en-tête (8 octets), signature du PNG
    size_t size = fread ( header, 1, 8, file );
    if ( png_sig_cmp ( header, 0, 8 ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Provided file is not recognized as a PNG file " << filename ;
        return NULL;
    }

    /* initialize stuff */
    pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!pngStruct) {
        BOOST_LOG_TRIVIAL(error) << "Cannot create the PNG reading structure for image " << filename;
        return NULL;
    }

    pngInfo = png_create_info_struct(pngStruct);
    if (!pngInfo) {
        BOOST_LOG_TRIVIAL(error) << "Cannot create the PNG informations structure for image " << filename;
        return NULL;
    }

    if (setjmp(png_jmpbuf(pngStruct))) {
        BOOST_LOG_TRIVIAL(error) << "Error during PNG image initialization " << filename;
        return NULL;
    }

    // Initialisation des strcutures de lecture du PNG
    png_init_io(pngStruct, file);
    png_set_sig_bytes(pngStruct, 8);
    png_read_info(pngStruct, pngInfo);

    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0;
    SampleFormat::eSampleFormat sf;
    png_byte color_type, bit_depth;

    width = png_get_image_width(pngStruct, pngInfo);
    height = png_get_image_height(pngStruct, pngInfo);
    color_type = png_get_color_type(pngStruct, pngInfo);
    bit_depth = png_get_bit_depth(pngStruct, pngInfo);
    if (bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8 (pngStruct);
        bit_depth = 8;
    }


    if (int(bit_depth) == 8) {
        sf = SampleFormat::UINT8;
    } else if (int(bit_depth) == 16) {
        sf = SampleFormat::UINT16;
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported PNG image bit depth (" << int(bit_depth) << ") for the image to read : " << filename;
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
    
    // Passage dans un format utilisable par la libimage

    switch ( int(color_type) ) {
        case PNG_COLOR_TYPE_GRAY :
        {
            BOOST_LOG_TRIVIAL(debug) << "Initial PNG color type PNG_COLOR_TYPE_GRAY";
            channels = 1;
            break;
        }
        case PNG_COLOR_TYPE_GRAY_ALPHA :
        {
            BOOST_LOG_TRIVIAL(debug) << "Initial PNG color type PNG_COLOR_TYPE_GRAY_ALPHA";
            channels = 2;
            break;            
        }
        case PNG_COLOR_TYPE_RGB :
        {
            BOOST_LOG_TRIVIAL(debug) << "Initial PNG color type PNG_COLOR_TYPE_RGB";
            channels = 3;
            break;
        }
        case PNG_COLOR_TYPE_PALETTE :
        {
            BOOST_LOG_TRIVIAL(debug) << "Initial PNG color type PNG_COLOR_TYPE_PALETTE";
            channels = 3;
            png_set_palette_to_rgb (pngStruct);
            break;
        }
        case PNG_COLOR_TYPE_RGB_ALPHA :
        {
            BOOST_LOG_TRIVIAL(debug) << "Initial PNG color type PNG_COLOR_TYPE_RGB_ALPHA";
            channels = 4;
            break;
        }
        default :
        {
            BOOST_LOG_TRIVIAL(error) << "Cannot interpret the color type (" << int(color_type) << ") for the PNG image " << filename;
            return NULL;
        }
    }
    
    if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS)) {
        BOOST_LOG_TRIVIAL(debug) << "Convert tRNS to alpha sample for PNG image";        
        png_set_tRNS_to_alpha(pngStruct);
        channels += 1;
    }
    
    png_read_update_info (pngStruct, pngInfo);

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    if (setjmp(png_jmpbuf(pngStruct))) {
        BOOST_LOG_TRIVIAL(error) << "Error reading PNG image " << filename;
        return NULL;
    }

    pngData = (png_bytep*) malloc(sizeof(png_bytep) * height);
    int rowbytes = png_get_rowbytes(pngStruct,pngInfo);
    for (int y = 0; y < height; y++) {
        pngData[y] = (png_byte*) malloc(rowbytes);
    }

    png_read_image(pngStruct, pngData);

    fclose ( file );
    png_destroy_read_struct(&pngStruct, &pngInfo, (png_infopp)NULL);
    
    /******************** CRÉATION DE L'OBJET ******************/
    
    return new LibpngImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, to_rok4_photometric ( color_type ), Compression::PNG,
        pngData
    );
    
}

void ReadDataFromInputStream(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {

   png_voidp io_ptr = png_get_io_ptr(png_ptr);
   if(io_ptr == NULL) {
       BOOST_LOG_TRIVIAL(error) << "Can't get io pointer";
       return;
    }

   RawDataStream& inputStream = *(RawDataStream*)io_ptr;
   const size_t bytesRead = inputStream.read((uint8_t*)outBytes,(size_t)byteCountToRead);

   if((png_size_t)bytesRead != byteCountToRead) {
        BOOST_LOG_TRIVIAL(error) << "Can't get data from stream";
        return;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibpngImage::LibpngImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric, Compression::eCompression compression,
    png_bytep* pngData ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sample_format, photometric, compression, ExtraSample::ALPHA_UNASSOC ),

    data(pngData) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibpngImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];

    for (int x = 0;  x < width * channels; x++) {
        buffertmp[x] = data[line][x];
    }

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convert_line(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixel_size * width);
    }
    
    return width * get_channels();
}


int LibpngImage::get_line ( uint8_t* buffer, int line ) {
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

int LibpngImage::get_line ( uint16_t* buffer, int line ) {
    
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

int LibpngImage::get_line ( float* buffer, int line ) {
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



