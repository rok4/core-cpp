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
 * \file LibtiffImage.cpp
 ** \~french
 * \brief Implémentation des classes LibtiffImage
 * \details
 * \li LibtiffImage : image physique, attaché à un fichier
 ** \~english
 * \brief Implement classes LibtiffImage
 * \details
 * \li LibtiffImage : physical image, linked to a file
 */

#include "image/file/LibtiffImage.h"
#include <boost/log/trivial.hpp>
#include <cstdint>
#include "utils/Utils.h"
#include "processors/OneBitConverter.h"


/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */


static SampleFormat::eSampleFormat to_rok4_sampleformat ( uint16_t sf, int bps) {

    if (sf == SAMPLEFORMAT_UINT && (bps == 8 || bps == 1)) {
        return SampleFormat::UINT8;
    } else if (sf == SAMPLEFORMAT_UINT && bps == 16) {
        return SampleFormat::UINT16;
    } else if (sf == SAMPLEFORMAT_IEEEFP && bps == 32) {
        return SampleFormat::FLOAT32;
    } else {
        return SampleFormat::UNKNOWN;
    }
}

static uint16_t from_rok4_sampleformat ( SampleFormat::eSampleFormat sf ) {
    switch ( sf ) {
    case SampleFormat::UINT8 :
        return SAMPLEFORMAT_UINT;
    case SampleFormat::UINT16 :
        return SAMPLEFORMAT_UINT;
    case SampleFormat::FLOAT32 :
        return SAMPLEFORMAT_IEEEFP;
    default :
        return 0;
    }
}

static Photometric::ePhotometric to_rok4_photometric ( uint16_t ph ) {
    switch ( ph ) {
    case PHOTOMETRIC_MINISBLACK :
        return Photometric::GRAY;
    case PHOTOMETRIC_MINISWHITE :
        return Photometric::GRAY;
    case PHOTOMETRIC_PALETTE :
        return Photometric::PALETTE;
    case PHOTOMETRIC_RGB :
        return Photometric::RGB;
    case PHOTOMETRIC_YCBCR :
        return Photometric::YCBCR;
    case PHOTOMETRIC_MASK :
        return Photometric::MASK;
    default :
        return Photometric::UNKNOWN;
    }
}

static uint16_t from_rok4_photometric ( Photometric::ePhotometric ph ) {
    switch ( ph ) {
    case Photometric::GRAY :
        return PHOTOMETRIC_MINISBLACK;
    case Photometric::RGB :
        return PHOTOMETRIC_RGB;
    case Photometric::PALETTE :
        return PHOTOMETRIC_PALETTE;
    case Photometric::YCBCR :
        return PHOTOMETRIC_YCBCR;
    case Photometric::MASK :
        return PHOTOMETRIC_MINISBLACK;
    default :
        return 0;
    }
}

static Compression::eCompression to_rok4_compression ( uint16_t comp ) {
    switch ( comp ) {
    case COMPRESSION_NONE :
        return Compression::NONE;
    case COMPRESSION_ADOBE_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_JPEG :
        return Compression::JPEG;
    case COMPRESSION_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_LZW :
        return Compression::LZW;
    case COMPRESSION_PACKBITS :
        return Compression::PACKBITS;
    default :
        return Compression::UNKNOWN;
    }
}

static uint16_t from_rok4_compression ( Compression::eCompression comp ) {
    switch ( comp ) {
    case Compression::NONE :
        return COMPRESSION_NONE;
    case Compression::DEFLATE :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::JPEG :
        return COMPRESSION_JPEG;
    case Compression::PNG :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::LZW :
        return COMPRESSION_LZW;
    case Compression::PACKBITS :
        return COMPRESSION_PACKBITS;
    default :
        return 0;
    }
}

static ExtraSample::eExtraSample to_rok4_extrasample ( uint16_t es ) {
    switch ( es ) {
    case EXTRASAMPLE_ASSOCALPHA :
        return ExtraSample::ALPHA_ASSOC;
    case EXTRASAMPLE_UNASSALPHA :
        return ExtraSample::ALPHA_UNASSOC;
    default :
        return ExtraSample::UNKNOWN;
    }
}

static uint16_t from_rok4_extrasample ( ExtraSample::eExtraSample es ) {
    switch ( es ) {
    case ExtraSample::ALPHA_ASSOC :
        return EXTRASAMPLE_ASSOCALPHA;
    case ExtraSample::ALPHA_UNASSOC :
        return EXTRASAMPLE_UNASSALPHA;
    default :
      return EXTRASAMPLE_UNSPECIFIED;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibtiffImage* LibtiffImage::create_to_read ( std::string filename, BoundingBox< double > bbox, double resx, double resy ) {

    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sf=0, ph=0, comp=0, rowsperstrip=0;
    bool tiled = false, palette = false;
    TIFF* tif = TIFFOpen ( filename.c_str(), "r" );


    /************** RECUPERATION DES INFORMATIONS **************/

    if ( tif == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open TIFF (to read) " << filename ;
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read pixel width for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_IMAGELENGTH, &height ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read pixel height for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_SAMPLESPERPIXEL,&channels ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read number of samples per pixel for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_PLANARCONFIG,&planarconfig ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read planar configuration for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( planarconfig != PLANARCONFIG_CONTIG && channels != 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Planar configuration have to be 'PLANARCONFIG_CONTIG' for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_BITSPERSAMPLE,&bitspersample ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read number of bits per sample for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_SAMPLEFORMAT,&sf ) < 1 ) {
        if ( bitspersample == 8 ) {
            sf = SAMPLEFORMAT_UINT;
        } else if ( bitspersample == 16 ) {
            sf = SAMPLEFORMAT_UINT;
        } else if ( bitspersample == 32 ) {
            sf = SAMPLEFORMAT_IEEEFP;
        } else if ( bitspersample == 1 ) {
            sf = SAMPLEFORMAT_UINT;
        } else {
            BOOST_LOG_TRIVIAL(error) <<  "Unable to determine sample format from the number of bits per sample (" << bitspersample << ") for file " << filename ;
            TIFFClose ( tif );
            return NULL;
        }
    }

    if ( TIFFGetField ( tif, TIFFTAG_PHOTOMETRIC,&ph ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read photometric for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if (to_rok4_photometric ( ph ) == Photometric::PALETTE) {
        palette = true;
    }

    if ( TIFFGetField ( tif, TIFFTAG_COMPRESSION,&comp ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read compression for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_ROWSPERSTRIP,&rowsperstrip ) < 1 ) {
        if ( TIFFGetField ( tif, TIFFTAG_TILELENGTH,&rowsperstrip ) < 1 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Unable to read number of rows per strip or tile length for file " << filename ;
            TIFFClose ( tif );
            return NULL;
        } else {
            tiled = true;
        }
    }

    ExtraSample::eExtraSample es = ExtraSample::NONE;
    uint16_t extrasamplesCount;
    uint16_t* extrasamples;
    if ( TIFFGetField ( tif, TIFFTAG_EXTRASAMPLES, &extrasamplesCount, &extrasamples ) > 0 ) {
        // On a des canaux en plus, si c'est de l'alpha (le premier extra), et qu'il est associé,
        // on le précise pour convertir à la volée lors de la lecture des lignes
        es = to_rok4_extrasample(extrasamples[0]);
        if ( es == ExtraSample::ALPHA_ASSOC ) {
            BOOST_LOG_TRIVIAL(info) <<  "Alpha sample is associated for the file " << filename << ". We will convert for reading";
        }
    }

    if (palette && es != ExtraSample::NONE) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot read image with color map and alpha channel (file " << filename << ")";
        TIFFClose ( tif );
        return NULL;
    }

    /********************** CONTROLES **************************/

    if ( to_rok4_sampleformat ( sf, bitspersample ) == SampleFormat::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported sample type : " << sf << " and " << bitspersample << " bits per sample" ;
        BOOST_LOG_TRIVIAL(error) <<  "\t for the image to read : " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" ;
            TIFFClose ( tif );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    /******************** CRÉATION DE L'OBJET ******************/

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, comp,
        tif, rowsperstrip, es, tiled, palette
    );
}


/* ----- Pour l'écriture ----- */

LibtiffImage* LibtiffImage::create_to_write (
    std::string filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sampleformat, Photometric::ePhotometric photometric,
    Compression::eCompression compression, uint16_t rowsperstrip ) {

    if (compression == Compression::JPEG && photometric == Photometric::RGB)
        photometric = Photometric::YCBCR;

    if (compression != Compression::JPEG && photometric == Photometric::YCBCR)
        photometric = Photometric::RGB;

    if ( width <= 0 || height <= 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "One dimension is not valid for the output image " << filename << " : " << width << ", " << height ;
        return NULL;
    }

    if ( channels <= 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Number of samples per pixel is not valid for the output image " << filename << " : " << channels ;
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche
        int calcWidth = lround ( ( bbox.xmax - bbox.xmin ) / ( resx ) );
        int calcHeight = lround ( ( bbox.ymax - bbox.ymin ) / ( resy ) );
        if ( calcWidth != width || calcHeight != height ) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" ;
            BOOST_LOG_TRIVIAL(error) <<  "Height is " << height << " and calculation give " << calcHeight ;
            BOOST_LOG_TRIVIAL(error) <<  "Width is " << width << " and calculation give " << calcWidth ;
            return NULL;
        }
    }

    TIFF* tif = TIFFOpen ( filename.c_str(), "w" );
    if ( tif == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open TIFF (to write) " << filename ;
        return NULL;
    }

    // Ecriture de l'en-tête pour récupérer les informations sur l'image
    if ( TIFFSetField ( tif, TIFFTAG_IMAGEWIDTH, width ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write pixel width for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_IMAGELENGTH, height ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write pixel height for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_SAMPLESPERPIXEL,channels ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write number of samples per pixel for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( channels == 4 || channels == 2 ) {
        uint16_t extrasample = EXTRASAMPLE_UNASSALPHA;
        if ( TIFFSetField ( tif, TIFFTAG_EXTRASAMPLES,1,&extrasample ) < 1 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Unable to write number of extra samples for file " << filename ;
            TIFFClose ( tif );
            return NULL;
        }
    }

    if ( TIFFSetField ( tif, TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write planar configuration for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_BITSPERSAMPLE, SampleFormat::get_bits_per_sample(sampleformat) ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write number of bits per sample for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_SAMPLEFORMAT, from_rok4_sampleformat ( sampleformat ) ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write sample format for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_PHOTOMETRIC, from_rok4_photometric ( photometric ) ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write photometric for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_COMPRESSION, from_rok4_compression ( compression ) ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write compression for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_ROWSPERSTRIP,rowsperstrip ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write number of rows per strip for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE ) < 1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to write pixel resolution unit for file " << filename ;
        TIFFClose ( tif );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and dimensions for image (to write)'" << filename << "' are not consistent" ;
            TIFFClose ( tif );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sampleformat, photometric, compression,
        tif, rowsperstrip, ExtraSample::ALPHA_UNASSOC
    );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEURS ---------------------------------------- */

LibtiffImage::LibtiffImage (
    int width,int height, double resx, double resy, int ch, BoundingBox<double> bbox, std::string name,
    int sf, int bps, int ph,
    int comp, TIFF* tif, int rowsperstrip, ExtraSample::eExtraSample extra_sample_type, bool tiled, bool palette) :

    FileImage ( width, height, resx, resy, ch, bbox, name, to_rok4_sampleformat( sf, bps ),
                to_rok4_photometric( ph ), to_rok4_compression( comp ), extra_sample_type
              ),

    tif ( tif ), rowsperstrip ( rowsperstrip ), tiled (tiled), palette (palette) {

    // Ce constructeur permet de déterminer si la conversion de 1 à 8 bits est nécessaire, et de savoir si 0 est blanc ou noir

    if ( bps == 1 ) {
        // On fera la conversion en entiers sur 8 bits à la volée.
        // Cette image sera comme une image sur 8 bits.
        // On change donc les informations, en précisant que la conversion doit être faite à la lecture.
        BOOST_LOG_TRIVIAL(debug) <<  "We have 1-bit samples for the file " << filename << ". We will convert for reading into 8-bit samples";
        pixel_size = channels;
        if (ph == PHOTOMETRIC_MINISWHITE) bit_to_byte = 1;
        else if (ph == PHOTOMETRIC_MINISBLACK) bit_to_byte = 2;
        else {
            BOOST_LOG_TRIVIAL(warning) << "Image '" << filename << "' has 1-bit sample and is not PHOTOMETRIC_MINISWHITE or PHOTOMETRIC_MINISWHITE ?";
            bit_to_byte = 0;
        }
    } else {
        bit_to_byte = 0;
    }

    if (palette) {
        // On fera la conversion en entiers à la volée.
        // On change donc les informations, pour exposer le format final.
        BOOST_LOG_TRIVIAL(debug) <<  "We have a color map for the file " << filename << ". We will convert for reading into RGB samples";
        channels = 3;
        pixel_size = channels;
        photometric = Photometric::RGB;
    }

    current_strip = -1;
    int stripSize = width*rowsperstrip*pixel_size;
    strip_buffer = new uint8_t[stripSize];

    if (bit_to_byte) {
        // On a besoin d'un buffer supplémentaire pour faire la conversion à la volée à la lecture
        bit_to_byte_buffer = new uint8_t[stripSize];
    }
}

LibtiffImage::LibtiffImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sampleformat, Photometric::ePhotometric photometric,
    Compression::eCompression compression, TIFF* tif, int rowsperstrip, ExtraSample::eExtraSample extra_sample_type) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, photometric, compression, extra_sample_type ),

    tif ( tif ), rowsperstrip ( rowsperstrip ) {

    bit_to_byte = 0;

    current_strip = -1;
    int stripSize = width*rowsperstrip*pixel_size;
    strip_buffer = new uint8_t[stripSize];
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibtiffImage::_getline ( T* buffer, int line ) {
    // buffer doit déjà être alloué, et assez grand, en tenant compte de la conversion

    if ( line / rowsperstrip != current_strip ) {

        // Les données n'ont pas encore été lue depuis l'image (strip pas en mémoire).
        current_strip = line / rowsperstrip;

        int size = -1;

        if (tiled) {
            int tile_width = 0,  tile_height = 0;
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);

            int tilenumber_widthwise = width / tile_width;
            if (width % tile_width != 0) {
                // On ajoute la tuile incomplète
                tilenumber_widthwise++;
            }

            int tile_size = tile_width * tile_height * pixel_size;
            int tile_row_size = tile_width * pixel_size;
            int last_tile_row_size = (width % tile_width) * pixel_size;

            int row_size = width * pixel_size;

            tdata_t tile_buf = _TIFFmalloc(tile_size);

            for (int t = 0; t < tilenumber_widthwise; t++) {

                // Si l'image a une photometrie Palette ou bien YCBCR --> TIFFReadRGBATile
                if (palette || photometric == Photometric::YCBCR) {
                    uint32* palette_buffer = new uint32[width * rowsperstrip];

                    size = TIFFReadRGBATile(tif, t * tile_width, current_strip * tile_height, palette_buffer );
                    if ( size == 0 ) {
                        BOOST_LOG_TRIVIAL(error) <<  "Cannot read tile " << t * tile_width << "," << current_strip * tile_height << " of image with color map " << filename ;
                        return 0;
                    }

                    if (channels == 4) {
                        BOOST_LOG_TRIVIAL(error) <<  "Cannot read image with color map and alpha channel (file " << filename << ")";
                        return 0;
                    } else if (channels == 3) {
                        // On ne doit pas garder le dernier octet de chaque entier sur 32 bits (canal alpha non présent)
                        for (int l = 0; l < rowsperstrip; l++) {
                            for (int i = 0; i < tile_width; i++) {
                                memcpy ( (uint8_t*) tile_buf + (tile_width * l + i) * pixel_size, palette_buffer + (rowsperstrip - 1 - l) * tile_width + i, 3 );
                            }
                        }
                    }

                    delete [] palette_buffer;
                } else {
                    ttile_t tile = current_strip * tilenumber_widthwise + t;
                    size = TIFFReadEncodedTile(tif, tile, tile_buf, -1);
                    if ( size < 0 ) {
                        BOOST_LOG_TRIVIAL(error) <<  "Cannot read tile number " << tile << " of image " << filename ;
                        return 0;
                    }
                }

                // On constitue la ligne à partir des lignes des tuiles
                int size_to_read = tile_row_size;
                if (t == tilenumber_widthwise - 1) {
                    // on lit la dernière tuile, potentiellement non complète
                    size_to_read = last_tile_row_size;
                }

                for ( int l = 0; l < rowsperstrip; l++ ) {
                    memcpy ( strip_buffer + l * row_size + t * tile_row_size, (uint8_t*) tile_buf + l * tile_row_size, size_to_read );
                }
            }

            _TIFFfree(tile_buf);
        } else {
            // Si l'image a une photometrie Palette ou bien YCBCR --> TIFFReadRGBAStrip
            if (palette || photometric == Photometric::YCBCR) {
                uint32* palette_buffer = new uint32[width * rowsperstrip];
                size = TIFFReadRGBAStrip ( tif, line, palette_buffer);
                if ( size == 0 ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Cannot read strip number " << current_strip << " of image with color map " << filename ;
                    return 0;
                }

                // Les lignes dans le strip lue ne sont pas dans le même ordre, mais sont indexées de bas en haut

                if (channels == 4) {
                    BOOST_LOG_TRIVIAL(error) <<  "Cannot read image with color map and alpha channel (file " << filename << ")";
                    return 0;
                } else if (channels == 3) {
                    // On ne doit pas garder le dernier octet de chaque entier sur 32 bits (canal alpha non présent)

                    // on va calculer le nombre de ligne dans le strip : pour le dernier, on peut avoir moins de lignes
                    int rows_count = rowsperstrip;
                    if (line / rowsperstrip == (height - 1) / rowsperstrip) {
                        rows_count = height % rowsperstrip;
                    }

                    for (int l = 0; l < rows_count; l++) {
                        for (int i = 0; i < width; i++) {
                            memcpy ( strip_buffer + (width * l + i) * pixel_size, palette_buffer + (rows_count - 1 - l) * width + i, 3 );
                        }
                    }
                }
                delete [] palette_buffer;
            } else {
                size = TIFFReadEncodedStrip ( tif, current_strip, strip_buffer, -1 );
                if ( size < 0 ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Cannot read strip number " << current_strip << " of image " << filename ;
                    return 0;
                }
            }
        }

        if (bit_to_byte == 1) {
            OneBitConverter::minwhiteToGray(bit_to_byte_buffer, strip_buffer, size);
        } else if (bit_to_byte == 2) {
            OneBitConverter::minblackToGray(bit_to_byte_buffer, strip_buffer, size);
        }
    }


    T buffertmp[width * channels];

    /************* SI CONVERSION 1 bit -> 8 bits **************/

    if (bit_to_byte) {
        memcpy ( buffertmp, bit_to_byte_buffer + ( line%rowsperstrip ) * width * pixel_size, width * pixel_size );
    } else {
        memcpy ( buffertmp, strip_buffer + ( line%rowsperstrip ) * width * pixel_size, width * pixel_size );
    }

    /********************* SI ALPHA ASSOCIE *******************/

    if (extra_sample == ExtraSample::ALPHA_ASSOC) unassociate_alpha ( buffertmp );

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convert_line(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixel_size * width);
    }


    return width * get_channels();
}

int LibtiffImage::get_line ( uint8_t* buffer, int line ) {
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

int LibtiffImage::get_line ( uint16_t* buffer, int line ) {

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

int LibtiffImage::get_line ( float* buffer, int line ) {
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

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */

int LibtiffImage::write_image ( Image* pIn ) {

    // Contrôle de la cohérence des 2 images : dimensions
    if ( width != pIn->get_width() || height != pIn->get_height() ) {
        BOOST_LOG_TRIVIAL(error) <<  "Image we want to write has not consistent dimensions with the output image" ;
        return -1;
    }

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT8 ) {
        uint8_t* buf_u = ( unsigned char* ) _TIFFmalloc ( width * pixel_size );
        for ( int line = 0; line < height; line++ ) {
            if (pIn->get_line ( buf_u,line ) == 0) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read input image line " << line;
                return -1;
            }
            if ( TIFFWriteScanline ( tif, buf_u, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }
        _TIFFfree ( buf_u );

    } else if ( sample_format == SampleFormat::UINT16 ) {
        uint16_t* buf_t = ( uint16_t* ) _TIFFmalloc ( width * pixel_size );
        for ( int line = 0; line < height; line++ ) {
            if (pIn->get_line ( buf_t,line ) == 0) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read input image line " << line;
                return -1;
            }
            if ( TIFFWriteScanline ( tif, buf_t, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }
        _TIFFfree ( buf_t );
    } else if ( sample_format == SampleFormat::FLOAT32 ) {
        float* buf_f = ( float* ) _TIFFmalloc ( width * pixel_size );
        for ( int line = 0; line < height; line++ ) {
            if (pIn->get_line ( buf_f,line ) == 0) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read input image line " << line;
                return -1;
            }
            if ( TIFFWriteScanline ( tif, buf_f, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }
        _TIFFfree ( buf_f );
    }

    return 0;
}

int LibtiffImage::write_image ( uint8_t* buffer) {

    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT8 ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }

    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (from a buffer) has not 8-bit uint samples : " << filename;
        print();
        return -1;
    }

    return 0;
}

int LibtiffImage::write_image ( uint16_t* buffer) {

    // Si l'image à écrire n'a pas des canaux en entiers sur 16 bits, on sort en erreur

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT16 ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }

    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (from a buffer) has not 16-bit uint samples : " << filename;
        print();
        return -1;
    }

    return 0;
}

int LibtiffImage::write_image ( float* buffer) {

    // Si l'image à écrire n'a pas des canaux en flottant sur 32 bits, on sort en erreur

    if ( sample_format == SampleFormat::FLOAT32 ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
                return -1;
            }
        }
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (from a buffer) has not 32-bit float samples : " << filename;
        print();
        return -1;
    }

    return 0;
}

int LibtiffImage::write_line ( uint8_t* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT8 ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
            return -1;
        }

    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (line by line) has not 8-bit uint samples : " << filename;
        print();
        return -1;
    }


    return 0;
}

int LibtiffImage::write_line ( uint16_t* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en entiers sur 16 bits, on sort en erreur

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT16 ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
            return -1;
        }

    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (line by line) has not 16-bit uint samples : " << filename;
        print();
        return -1;
    }

    return 0;
}

int LibtiffImage::write_line ( float* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en flottant sur 32 bits, on sort en erreur

    if ( sample_format == SampleFormat::FLOAT32 ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot write file " << TIFFFileName ( tif ) << ", line " << line ;
            return -1;
        }
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Image to write (line by line) has not 32-bit float samples : " << filename;
        print();
        return -1;
    }

    return 0;
}
