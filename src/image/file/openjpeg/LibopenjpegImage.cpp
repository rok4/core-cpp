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
 * \file LibopenjpegImage.cpp
 ** \~french
 * \brief Implémentation des classes LibopenjpegImage
 * \details
 * \li LibopenjpegImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 ** \~english
 * \brief Implement classes LibopenjpegImage
 * \details
 * \li LibopenjpegImage : manage a JPEG2000 format image, reading, using the library openjpeg
 */

#include <string.h>
#include <cstring>
#include <ctype.h>

#include "image/file/openjpeg/LibopenjpegImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */

static Photometric::ePhotometric to_rok4_photometric ( OPJ_COLOR_SPACE ph, int channels ) {
    switch ( ph ) {
    case OPJ_CLRSPC_SRGB :
        return Photometric::RGB;
    case OPJ_CLRSPC_GRAY :
        return Photometric::GRAY;
    case OPJ_CLRSPC_UNSPECIFIED :
        switch(channels) {
            case 1:
                return Photometric::GRAY;
            case 2:
                return Photometric::GRAY;
            case 3:
                return Photometric::RGB;
            case 4:
                return Photometric::RGB;
            default:
                return Photometric::UNKNOWN;
        }
        
    default :
        return Photometric::UNKNOWN;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------- LOGGERS DE OPENJPEG ------------------------------------- */

static void error_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(error) <<  msg ;
}

static void warning_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(warning) <<  msg ;
}

static void info_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(debug) <<  msg ;
}

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibopenjpegImage* LibopenjpegImage::create_to_read ( std::string filename, BoundingBox< double > bbox, double resx, double resy ) {

    // Set decoding parameters to default values
    opj_dparameters_t parameters;
    opj_image_t* image = NULL;
    opj_stream_t* stream = NULL;
    opj_codec_t* codec = NULL;

    opj_set_default_decoder_parameters ( &parameters );
    strncpy ( parameters.infile, filename.c_str(), IMAGE_MAX_FILENAME_LENGTH * sizeof ( char ) );

    /************** INITIALISATION DES OBJETS OPENJPEG *********/

    // Ouverture du fichier binaire en lecture
    FILE *file = NULL;
    file = std::fopen ( filename.c_str() , "rb" );

    if ( !file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open the JPEG2000 file (to read) " << filename ;
        return NULL;
    }

    // Récupération du format du JPEG2000 (magic code) pour savoir quel codec utiliser pour la décompression
    unsigned char * magic_code = ( unsigned char * ) malloc ( 12 );

    if ( std::fread ( magic_code, 1, 12, file ) != 12 ) {
        free ( magic_code );
        std::fclose ( file );
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read the magic code for the JPEG2000 file " << filename ;
        return NULL;
    }

    std::fclose ( file );

    // Format MAGIC Code
    OPJ_CODEC_FORMAT codec_format;
    if ( memcmp ( magic_code, JP2_RFC3745_MAGIC, 12 ) == 0 || memcmp ( magic_code, JP2_MAGIC, 4 ) == 0 ) {
        codec_format = OPJ_CODEC_JP2;
        BOOST_LOG_TRIVIAL(debug) <<  "Ok, use format JP2 !" ;
    } else if ( memcmp ( magic_code, J2K_CODESTREAM_MAGIC, 4 ) == 0 ) {
        codec_format = OPJ_CODEC_J2K;
        BOOST_LOG_TRIVIAL(debug) <<  "Ok, use format J2K !" ;
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled format for the JPEG2000 file " << filename ;
        free ( magic_code );
        return NULL;
    }

    codec = opj_create_decompress ( codec_format );

    // Nettoyage
    free ( magic_code );

    /* catch events using our callbacks and give a local context */
    opj_set_info_handler ( codec, info_callback,00 );
    opj_set_warning_handler ( codec, warning_callback,00 );
    opj_set_error_handler ( codec, error_callback,00 );

    /* Setup the decoder decoding parameters using user parameters */
    if ( !opj_setup_decoder ( codec, &parameters ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to setup the decoder for the JPEG2000 file " << filename ;
        opj_destroy_codec ( codec );
        return NULL;
    }

    stream = opj_stream_create_default_file_stream ( filename.c_str(),1 );
    if ( !stream ) {
        std::fclose ( file );
        BOOST_LOG_TRIVIAL(error) <<  "Unable to create the stream (to read) for the JPEG2000 file " << filename ;
        return NULL;
    }

    /* Read the main header of the codestream and if necessary the JP2 boxes*/
    if ( ! opj_read_header ( stream, codec, &image ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read the header for the JPEG2000 file " << filename ;
        opj_stream_destroy ( stream );
        opj_destroy_codec ( codec );
        opj_image_destroy ( image );
        return NULL;
    }
    
    /************** RECUPERATION DES INFORMATIONS **************/

    int bitspersample = image->comps[0].prec;
    int width = image->comps[0].w;
    int height = image->comps[0].h;
    int channels = image->numcomps;
    int tile_width = 0;
    int rowsperstrip = 0;

    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;

    switch (bitspersample){
        case 8:
            sf = SampleFormat::UINT8;
            break;
        
        default:
            break;
    }

    Photometric::ePhotometric ph = to_rok4_photometric ( image->color_space , channels);
    if ( ph == Photometric::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled color space (" << image->color_space << ") in the JPEG2000 image " << filename ;
        return NULL;
    }

    // On vérifie que toutes les composantes ont bien les mêmes carctéristiques
    for ( int i = 1; i < channels; i++ ) {
        if ( bitspersample != image->comps[i].prec || width != image->comps[i].w || height != image->comps[i].h ) {
            BOOST_LOG_TRIVIAL(error) <<  "All components have to be the same in the JPEG image " << filename ;
            return NULL;
        }
    }

    opj_codestream_info_v2_t *jp2_code_stream_info = opj_get_cstr_info(codec);
    if (jp2_code_stream_info->tw == 1 && jp2_code_stream_info->th == 1) {
        tile_width = 0;
        rowsperstrip = 512;
    } else {
        tile_width = jp2_code_stream_info->tdx;
        rowsperstrip = jp2_code_stream_info->tdy;
    }
    opj_destroy_cstr_info(&jp2_code_stream_info);

    opj_stream_destroy ( stream );
    opj_image_destroy( image );
    opj_destroy_codec ( codec );

    /********************** CONTROLES **************************/

    if ( sf == SampleFormat::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported JPEG2000 with " << bitspersample << " bits per sample bands for the image to read : " << filename ;
        return NULL;
    }
    
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

    /******************** CRÉATION DE L'OBJET ******************/

    return new LibopenjpegImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, ph, Compression::JPEG2000,
        parameters, codec_format, rowsperstrip, tile_width
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibopenjpegImage::LibopenjpegImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sampleformat, Photometric::ePhotometric photometric, Compression::eCompression compression,
    opj_dparameters_t parameters, OPJ_CODEC_FORMAT codec_format, int rowsperstrip, int tw ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, photometric, compression ),

    jp2_parameters ( parameters ), jp2_codec(codec_format), rowsperstrip(rowsperstrip), tile_width(tw) {

    current_strip = -1;
    int strip_size = width*rowsperstrip*pixel_size;
    strip_buffer = new uint8_t[strip_size];
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibopenjpegImage::_getline ( T* buffer, int line ) {

    if ( line / rowsperstrip != current_strip ) {

        opj_image_t* image = NULL;
        opj_stream_t* stream = NULL;
        opj_codec_t* codec = NULL;

        // Les données n'ont pas encore été lue depuis l'image (strip pas en mémoire).
        current_strip = line / rowsperstrip;
        int row_size = width * pixel_size;

        if (tile_width != 0) {

            int tilenumber_widthwise = ceil((double) width / tile_width);
            int tilenumber_heightwise = ceil((double) height / rowsperstrip);
            int tile_row_size = tile_width * pixel_size;
            int tile_size = tile_width * rowsperstrip * pixel_size;

            for (int t = 0; t < tilenumber_widthwise; t++) {
                OPJ_UINT32 tile_index = tilenumber_widthwise * current_strip + t;

                codec = opj_create_decompress ( jp2_codec );

                /* catch events using our callbacks and give a local context */
                opj_set_info_handler ( codec, info_callback,00 );
                opj_set_error_handler ( codec, error_callback,00 );

                /* Setup the decoder decoding parameters using user parameters */
                if ( ! opj_setup_decoder ( codec, &jp2_parameters ) ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unable to setup the decoder to read the tile of the JPEG2000 file " << filename ;
                    return 0;
                }

                stream = opj_stream_create_default_file_stream ( filename.c_str(),1 );
                if ( ! stream ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unable to create the stream to read the tile of the JPEG2000 file " << filename ;
                    return 0;
                }

                /* Read the main header of the codestream and if necessary the JP2 boxes*/
                if ( ! opj_read_header ( stream, codec, &image ) ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unable to read the header to read the tile of the JPEG2000 file " << filename ;
                    return 0;
                }

                if ( ! ( opj_get_decoded_tile(codec, stream, image, tile_index) && opj_end_decompress ( codec, stream ) ) ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Cannot read tile number " << tile_index << " of image " << filename ;
                    return 0;
                }

                int current_width = tile_width;
                if (t == tilenumber_widthwise - 1) {
                    // on lit la dernière tuile, potentiellement non complète
                    current_width = width - (tilenumber_widthwise - 1) * tile_width;
                }

                int current_height = rowsperstrip;
                if (current_strip == tilenumber_heightwise - 1) {
                    // on lit la dernière ligne de tuile, potentiellement non complète
                    current_height = height - (tilenumber_heightwise - 1) * rowsperstrip;
                }

                for (int l = 0; l < current_height; l++) {
                    for (int i = 0; i < current_width; i++) {
                        int index = current_width * l + i;
                        for (int j = 0; j < channels; j++) {
                            *( (T*) (strip_buffer + l * row_size + t * tile_row_size + i * pixel_size + j * sizeof(T)) ) = image->comps[j].data[index];
                        }
                    }
                }

                opj_stream_destroy ( stream );
                opj_image_destroy( image );
                opj_destroy_codec ( codec );
            }

        } else {
            /* Get the decoded area */

            codec = opj_create_decompress ( jp2_codec );

            /* catch events using our callbacks and give a local context */
            opj_set_info_handler ( codec, info_callback,00 );
            opj_set_error_handler ( codec, error_callback,00 );

            /* Setup the decoder decoding parameters using user parameters */
            if ( !opj_setup_decoder ( codec, &jp2_parameters ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Unable to setup the decoder to read the area of the JPEG2000 file " << filename ;
                return 0;
            }

            stream = opj_stream_create_default_file_stream ( filename.c_str(),1 );
            if ( ! stream ) {
                BOOST_LOG_TRIVIAL(error) <<  "Unable to create the stream to read the area of the JPEG2000 file " << filename ;
                return 0;
            }

            /* Read the main header of the codestream and if necessary the JP2 boxes*/
            if ( ! opj_read_header ( stream, codec, &image ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Unable to read the header to read the area of the JPEG2000 file " << filename ;
                return 0;
            }

            if ( ! ( opj_set_decode_area(codec, image, 0, current_strip * rowsperstrip, width, std::min((current_strip + 1) * rowsperstrip, height))) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read area " << current_strip << " of image " << filename ;
                return 0;
            }

            /* Get the decoded image */
            if ( ! ( opj_decode ( codec, stream, image ) && opj_end_decompress ( codec, stream ) ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read strip number " << current_strip << " of image " << filename ;
                return 0;
            }

            // on prend la hauteur de la zone (area) en cours suite au decoupage afin d'eviter des
            // appels hors limites
            int current_rowsperstrip = image->comps[0].h;
            for (int l = 0; l < current_rowsperstrip; l++) {
                for (int i = 0; i < width; i++) {
                    int index = width * (l % current_rowsperstrip) + i;
                    for (int j = 0; j < channels; j++) {
                        *( (T*) (strip_buffer + l * row_size + i * pixel_size + j * sizeof(T)) ) = image->comps[j].data[index];
                    }
                }
            }

            opj_stream_destroy ( stream );
            opj_image_destroy( image );
            opj_destroy_codec ( codec );
        }
    }

    T buffertmp[width * channels];
    memcpy ( buffertmp, strip_buffer + ( line%rowsperstrip ) * width * pixel_size, width * pixel_size );

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convert_line(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixel_size * width);
    }
    
    return width * get_channels();
}


int LibopenjpegImage::get_line ( uint8_t* buffer, int line ) {
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

int LibopenjpegImage::get_line ( uint16_t* buffer, int line ) {
    
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

int LibopenjpegImage::get_line ( float* buffer, int line ) {
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





