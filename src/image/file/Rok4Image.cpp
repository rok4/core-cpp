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
 * \file Rok4Image.h
 ** \~french
 * \brief Implémentation des classes Rok4Image
 * \details
 * \li Rok4Image : gestion d'une image aux spécifications ROK4 Server (TIFF tuilé), en écriture et lecture
 ** \~english
 * \brief Implement classes Rok4Image
 * \details
 * \li Rok4Image : manage a ROK4 Server specifications image (tiled TIFF), reading and writting
 */

#include "image/file/Rok4Image.h"
#include "byteswap.h"
#include "compressors/LzwCompressor.h"
#include "compressors/PkbCompressor.h"
#include "datasource/StoreDataSource.h"
#include "datasource/Decoder.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"
#include "datasource/DataSource.h"
#include "utils/Cache.h"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------- Fonctions pour le manager de sortie de la libjpeg -------------------- */

void init_destination ( jpeg_compress_struct *cinfo ) {
    return;
}
boolean empty_output_buffer ( jpeg_compress_struct *cinfo ) {
    return false;
}
void term_destination ( jpeg_compress_struct *cinfo ) {
    return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------- Fonctions pour écrire des TIFFTAG dans l'en-tête --------------------- */

static inline void writeTIFFTAG (char** p,  uint16_t tag, uint16_t tagFormat, uint32_t card, uint32_t value ) {
    * ( ( uint16_t* ) *p ) = tag;
    * ( ( uint16_t* ) (*p + 2) ) = tagFormat;
    * ( ( uint32_t* ) (*p + 4) ) = card;
    * ( ( uint32_t* ) (*p + 8) ) = value;
    *p += 12;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONSTANTES ------------------------------------------ */

static const uint8_t PNG_IEND[12] = {
    0, 0, 0, 0, 'I', 'E', 'N', 'D',        // 8  | taille et type du chunck IHDR
    0xae, 0x42, 0x60, 0x82
}; // crc32


static const uint8_t PNG_HEADER[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,       // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',       // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                            // 16 | width
    0, 0, 1, 0,                            // 20 | height
    8,                                     // 24 | bit depth
    0,                                     // 25 | Colour type
    0,                                     // 26 | Compression method
    0,                                     // 27 | Filter method
    0,                                     // 28 | Interlace method
    0, 0, 0, 0
}; // 29 | crc32

static const uint8_t white[4] = {255,255,255,255};

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */


static SampleFormat::eSampleFormat to_rok4_sampleformat ( uint16_t sf, int bps ) {
    if (sf == SAMPLEFORMAT_UINT && bps == 8) {
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
    case Compression::JPEG90 :
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
    case ExtraSample::UNKNOWN :
        return EXTRASAMPLE_UNSPECIFIED;
    }
    return EXTRASAMPLE_UNSPECIFIED;
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

Rok4Image* Rok4Image::create_to_read ( std::string name, BoundingBox< double > bbox, double resx, double resy, Context* c ) {

    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sf=0, ph=0, comp=0;
    int tile_width=0, tile_height=0;
    
    // On va lire toutes les informations de l'en-tête TIFF à la main, sans passer par la libtiff pour être libre quant au type de stockage de la donnée
    
    StoreDataSource* sds = new StoreDataSource(name, c, 0, ROK4_IMAGE_HEADER_SIZE, "");

    size_t tmpSize;
    const uint8_t* hdr = sds->get_data(tmpSize);
    if ( hdr == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot read header of Rok4Image " << name ;
        return NULL;
    }
    if ( tmpSize < ROK4_IMAGE_HEADER_SIZE ) {

        BOOST_LOG_TRIVIAL(error) << tmpSize ;

        // S'il s'agit potentiellement d'un objet lien, on verifie d'abord que la signature de ce type d'objet est bien presente dans le header
        if ( strncmp((char*) hdr, ROK4_SYMLINK_SIGNATURE, ROK4_SYMLINK_SIGNATURE_SIZE) != 0 ) {
            BOOST_LOG_TRIVIAL(error) <<  "Erreur lors de la lecture du header, l'objet " << name << " ne correspond pas à un objet lien " ;
            delete[] hdr;
            return NULL;
        }

        std::string originalName (name);
        std::string originalTrayName (c->get_tray());

        char tmpName[tmpSize-ROK4_SYMLINK_SIGNATURE_SIZE+1];
        memcpy((uint8_t*) tmpName, hdr+ROK4_SYMLINK_SIGNATURE_SIZE,tmpSize-ROK4_SYMLINK_SIGNATURE_SIZE);
        tmpName[tmpSize-ROK4_SYMLINK_SIGNATURE_SIZE] = '\0';
        std::string full_name = std::string (tmpName);
        delete sds;

        std::string tray_name;
        name = full_name;
        if (c->get_type() != ContextType::FILECONTEXT) {
            // Dans le cas du stockage objet, on sépare le nom du contenant du nom de l'objet
            std::stringstream ss(full_name);
            std::string token;
            char delim = '/';
            std::getline(ss, token, delim);
            tray_name = token;
            name.erase(0, tray_name.length() + 1);
        }

        if (originalTrayName != tray_name) {
            // Récupération ou ajout du nouveau contexte de stockage
            c = StoragePool::get_context(c->get_type(), tray_name);
            // Problème lors de l'ajout ou de la récupération de ce contexte de stockage
            if (c == NULL) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot get target context for slab " << full_name ;
                return NULL;
            }
        }

        BOOST_LOG_TRIVIAL(debug) <<  "Symbolic slab detected : " << originalName << " -> " << full_name ;

        sds = new StoreDataSource(name, c, 0, ROK4_IMAGE_HEADER_SIZE, "");
        hdr = sds->get_data(tmpSize);

        if ( hdr == NULL) {
            BOOST_LOG_TRIVIAL(error) <<  "Erreur lors de la lecture du header et de l'index dans l'objet/fichier " << name ;
            delete sds;
            return NULL;
        }
        if ( tmpSize < ROK4_IMAGE_HEADER_SIZE ) {
            BOOST_LOG_TRIVIAL(error) <<  "Erreur lors de la lecture : une dalle symbolique " << originalName << " référence une autre dalle symbolique " << name ;
            delete sds;
            return NULL;
        }
    }
    

    uint8_t* p;
    
    /**************** DIMENSIONS GLOBALES ****************/
    p = ((uint8_t*) hdr)+26;
    width = *((uint32_t*) p);

    p = ((uint8_t*) hdr)+38;
    height = *((uint32_t*) p);

    /********************** TUILAGE **********************/

    p = ((uint8_t*) hdr)+98;
    tile_width = *((uint32_t*) p);

    p = ((uint8_t*) hdr)+110;
    tile_height = *((uint32_t*) p);

    /************ FORMAT DES PIXELS ET CANAUX ************/
    p = ((uint8_t*) hdr)+86;
    channels = *((uint32_t*) p);

    p = ((uint8_t*) hdr)+8;
    bitspersample = *((uint16_t*) p);

    p = ((uint8_t*) hdr)+74;
    ph = *((uint16_t*) p);

    p = ((uint8_t*) hdr)+62;
    comp = *((uint32_t*) p);
    
    // extrasample : facultatif
    p = ((uint8_t*) hdr)+138;
    uint16_t tagEs = *((uint16_t*) p);
    
    ExtraSample::eExtraSample es = ExtraSample::NONE;
    if (tagEs == TIFFTAG_EXTRASAMPLES) {
        p = ((uint8_t*) hdr)+146;
        es = to_rok4_extrasample(*((uint32_t*) p));
        
        p = ((uint8_t*) hdr)+158;
        sf = *((uint32_t*) p);
    } else if (tagEs == TIFFTAG_SAMPLEFORMAT) {
        p = ((uint8_t*) hdr)+146;
        sf = *((uint32_t*) p);        
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Inconsistent TIFF tag " << tagEs ;
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read sample format or extra samples for file " << name ;
        delete sds;
        return NULL;
    }
    
    delete sds;
    
    /********************** CONTROLES **************************/

    if ( to_rok4_sampleformat ( sf, bitspersample ) == SampleFormat::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported sample type : format (" << sf << ") and " << bitspersample << " bits per sample" <<  " for the image to read : " << name ;
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and real dimensions for image '" << name << "' are not consistent" ;
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    Rok4Image* ri = new Rok4Image (
        width, height, resx, resy, channels, bbox, name,
        to_rok4_sampleformat( sf, bitspersample ), to_rok4_photometric ( ph ), to_rok4_compression ( comp ), es,
        tile_width, tile_height, c
    );

    if ( ! ri->load_index() ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot load index of Rok4Image " << name ;
        return NULL;
    }

    return ri;
}

Rok4Image* Rok4Image::create_to_write (
    std::string name, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric,
    Compression::eCompression compression, int tile_width, int tile_height, Context* c  ) {

    if (width % tile_width != 0 || height % tile_height != 0) {
        BOOST_LOG_TRIVIAL(error) << "Image's dimensions have to be a multiple of tile's dimensions";
        return NULL;
    }

    if (compression == Compression::JPEG || compression == Compression::JPEG90) {
        if (channels != 3 || (photometric != Photometric::RGB && photometric != Photometric::YCBCR)) {
            BOOST_LOG_TRIVIAL(error) << "Only RGB images could be compressed with JPEG";
            return NULL;
        }

        if (sample_format != SampleFormat::UINT8) {
            BOOST_LOG_TRIVIAL(error) << "JPEG compression just handle 8-bits integer samples";
            return NULL;
        }

        photometric = Photometric::YCBCR;
    } else {
        if (photometric == Photometric::YCBCR) {
            photometric = Photometric::RGB;
        }
    }

    if (compression == Compression::PNG) {
        if (sample_format != SampleFormat::UINT8) {
            BOOST_LOG_TRIVIAL(error) << "PNG compression just handle 8-bits integer samples";
            return NULL;
        }
    }
    
    if ( resx > 0 && resy > 0 ) {
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and dimensions for the ROK4 image (to write)'" << name << "' are not consistent" ;
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new Rok4Image (
        width, height, resx, resy, channels, bbox, name,
        sample_format, photometric, compression, ExtraSample::ALPHA_UNASSOC, tile_width, tile_height, c
    );


}

Rok4Image* Rok4Image::create_to_write (
    std::string name, int tilePerWidth, int tilePerHeight, Context* c  ) {

    return new Rok4Image ( name, tilePerWidth, tilePerHeight, c );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

Rok4Image::Rok4Image (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string n,
    SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric,
    Compression::eCompression compression, ExtraSample::eExtraSample es, int tile_width, int tile_height, Context* c ) :

    Image ( width, height, channels, resx, resy, bbox),
    is_vector(false), sample_format ( sample_format ), photometric ( photometric ), compression ( compression ), extra_sample(es),
    tile_width (tile_width), tile_height(tile_height), context(c)
{

    name = n;
    pixel_size = SampleFormat::get_bits_per_sample(sample_format) * channels / 8;

    tiles_widthwise = width/tile_width;
    tiles_heightwise = height/tile_height;
    tiles_count = tiles_widthwise * tiles_heightwise;

    raw_tile_size = tile_width * tile_height * pixel_size;

    raw_tile_line_size = tile_width * pixel_size;


    memorized_tiles = new uint8_t[tiles_widthwise * raw_tile_size];
    memset ( memorized_tiles, 0, tiles_widthwise * raw_tile_size * sizeof ( uint8_t ) );

    memorized_tiles_line = -1;

}

Rok4Image::Rok4Image ( std::string n, int tpw, int tph, Context* c ) :

    Image ( 1, 1, 0, 1.0, 1.0, BoundingBox<double> ( 0.0, 0.0, 1.0, 1.0 )),
    is_vector ( true ), context(c),
    sample_format ( SampleFormat::UNKNOWN ), photometric ( Photometric::UNKNOWN ), 
    compression ( Compression::UNKNOWN ), extra_sample(ExtraSample::UNKNOWN),
    tile_width (tile_width), tile_height(tile_height)
{

    name = n;

    tiles_widthwise = tpw;
    tiles_heightwise = tph;
    tiles_count = tiles_widthwise * tiles_heightwise;

    pixel_size = 0;
    raw_tile_size = 0;
    raw_tile_line_size = 0;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

boolean Rok4Image::memorize_raw_tiles ( int tilesLine )
{    

    if ( tilesLine < 0 || tilesLine >= tiles_heightwise ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unvalid tiles' line indice (" << tilesLine << "). Have to be between 0 and " << tiles_heightwise-1 ;
        return false;
    }

    if (memorized_tiles_line == tilesLine) {
        return true;
    }

    /*
    On va récupérer l'offset de la première tuile de la ligne, ainsi que calculer la taille totale des tuiles de la ligne
    pour faire la lecture en une seule fois.
    Les tuiles ne sont pas parfaitement jointes sur le stockage, car les offset sont callées sur des multiples de 16
    */
    int firstTileIndex = tilesLine * tiles_widthwise;
    int firstTileOffset = tiles_offsets[firstTileIndex];

    int lastTileIndex = firstTileIndex + tiles_widthwise - 1;
    int lastTileOffset = tiles_offsets[lastTileIndex];
    int lastTileSize = tiles_sizes[lastTileIndex];

    StoreDataSource* totalDS = new StoreDataSource (name.c_str(), context, firstTileOffset, lastTileOffset - firstTileOffset + lastTileSize, "");
    size_t total_size;
    const uint8_t* enc_data = totalDS->get_data(total_size);
    if (enc_data == NULL) {
        BOOST_LOG_TRIVIAL(error) << "Cannot read tiles line data";
        return false;
    }

    // On va maintenant décompresser chaque tuile pour la stocker au format brut dans le buffer memorized_tiles
    for (size_t i = 0; i < tiles_widthwise; i++) {
        // Pour avoir l'offset de lecture de la tuile à décoder dans le buffer total, on utilise l'offset dans la dalle, 
        // en déduisant l'offset de la première tuile (qui correspond au 0 de notre buffer total)
        RawDataSource* encDS = new RawDataSource ( enc_data + tiles_offsets[firstTileIndex + i] - firstTileOffset, tiles_sizes[firstTileIndex + i]);

        DataSource* decDS;
        size_t tmpSize;

        if ( compression == Compression::NONE ) {
            decDS = encDS;
        }
        else if ( compression == Compression::JPEG ) {
            decDS = new DataSourceDecoder<JpegDecoder> ( encDS );
        }
        else if ( compression == Compression::JPEG90 ) {
            decDS = new DataSourceDecoder<JpegDecoder> ( encDS );
        }
        else if ( compression == Compression::LZW ) {
            decDS = new DataSourceDecoder<LzwDecoder> ( encDS );
        }
        else if ( compression == Compression::PACKBITS ) {
            decDS = new DataSourceDecoder<PackBitsDecoder> ( encDS );
        }
        else if ( compression == Compression::DEFLATE || compression == Compression::PNG ) {
            /* Avec une telle compression dans l'en-tête TIFF, on peut avoir :
             *       - des tuiles compressée en deflate (format "officiel")
             *       - des tuiles en PNG, format propre à ROK4
             * Pour distinguer les deux cas (pas le même décodeur), on va tester la présence d'un en-tête PNG */
            const uint8_t* header = encDS->get_data(tmpSize);
            if (header == NULL) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot read header to discrimine PNG and DEFLATE" ;
                return false;
            }
            if (memcmp(PNG_HEADER, header, 8)) {
                decDS = new DataSourceDecoder<DeflateDecoder> ( encDS );
            } else {
                compression = Compression::PNG;
                decDS = new DataSourceDecoder<PngDecoder> ( encDS );
            }
        }
        else {
            BOOST_LOG_TRIVIAL(error) <<  "Unhandled compression : " << compression ;
            return false;
        }

        const uint8_t* dec_data = decDS->get_data(tmpSize);
        
        if (! dec_data || tmpSize == 0) {
            BOOST_LOG_TRIVIAL(error) << "Unable to decompress tile " << i << " of tiles- line " << tilesLine;
            return false;
        } else if (tmpSize != raw_tile_size) {
            BOOST_LOG_TRIVIAL(warning) << "Raw tile size should have been " << raw_tile_size << ", and not " << tmpSize;
        }

        memcpy(memorized_tiles + i * raw_tile_size, dec_data, raw_tile_size );

        delete decDS;
    }

    delete totalDS;

    memorized_tiles_line = tilesLine;
    return true;
}

template <typename T>
int Rok4Image::_getline ( T* buffer, int line ) {
    int tilesLine = line / tile_height;
    int tileLine = line % tile_height;

    if (! memorize_raw_tiles (tilesLine)) {
        BOOST_LOG_TRIVIAL(error) << "Cannot read tiles to build line";
        return 0;
    }

    // Taille d'une ligne de tuile en nombre de case de type T
    int typetTileLineSize = tile_width * pixel_size / sizeof(T);

    // On constitue la ligne à partir des lignes des tuiles
    for ( int tileCol = 0; tileCol < tiles_widthwise; tileCol++ ) {
        memcpy ( buffer + tileCol * typetTileLineSize, memorized_tiles + tileCol * raw_tile_size + tileLine * raw_tile_line_size, raw_tile_line_size );
    }

    return tiles_widthwise * typetTileLineSize;
}

int Rok4Image::get_line ( uint8_t* buffer, int line ) {
    return _getline(buffer, line);
}

int Rok4Image::get_line ( uint16_t* buffer, int line ) {
    
    if ( sample_format == SampleFormat::UINT8 ) {
        // On veut la ligne en entiers 16 bits mais l'image lue est sur des entiers 8 bits
        // On convertit
        uint8_t* buffer_t = new uint8_t[width*channels];
        if (_getline(buffer_t, line) == 0) {
            return 0;
        }
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width * channels;
    } else if ( sample_format == SampleFormat::UINT16 ) {
        return _getline(buffer, line);   
    } else if ( sample_format == SampleFormat::FLOAT32 ) {
        // La donnée est en float mais on la veut sur des entiers 16 bits : on met donc un float sur deux entiers 16 bits
        if (_getline(buffer, line) == 0) {
            return 0;
        }
        return width * channels * 2;
    }

    return 0;
}

int Rok4Image::get_line ( float* buffer, int line ) {

    if ( sample_format == SampleFormat::UINT8 ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers sur 8 bits
        // On convertit
        uint8_t* buffer_t = new uint8_t[width*channels];
        if (_getline(buffer_t, line) == 0) {
            return 0;
        }
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width*channels;
    } else if ( sample_format == SampleFormat::UINT16 ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers sur 16 bits
        // On convertit
        uint16_t* buffer_t = new uint16_t[width*channels];
        if (_getline(buffer_t, line) == 0) {
            return 0;
        }
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width*channels;
    } else if ( sample_format == SampleFormat::FLOAT32 ) {
        return _getline(buffer, line);  
    }

    return width * channels;
}

bool Rok4Image::load_index()
{

    tiles_offsets = new uint32_t[tiles_count];
    tiles_sizes = new uint32_t[tiles_count];

    StoreDataSource* sds = new StoreDataSource (name, context, ROK4_IMAGE_HEADER_SIZE, 2 * 4 * tiles_count, "");

    size_t tmpSize;
    uint32_t* index = (uint32_t*) sds->get_data(tmpSize);
    if ( tmpSize !=  2 * 4 * tiles_count ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot read index of Rok4Image " << name ;
        return false;
    }

    for (int i = 0; i < tiles_count; i++) {
        tiles_offsets[i] = *(index + i);
        tiles_sizes[i] = *(index + tiles_count + i);
    }

    delete sds;

    return true;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

/** \todo Écriture d'images ROK4 en JPEG gris */
int Rok4Image::write_image ( Image* pIn )
{
    if (is_vector) {
        BOOST_LOG_TRIVIAL(error) << "Write image like that is not possible for vector slab";
        return -1;
    }
    
    if (! write_header()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot write the ROK4 images header for " << name;
        return -1;
    }

    if (! prepare_buffers()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot initialize buffers for " << name;
        return -1;
    }

    int imageLineSize = width * channels;
    int tileLineSize = tile_width * channels;
    uint8_t* tile = new uint8_t[tile_height*raw_tile_line_size];

    // Ecriture de l'image
    if ( sample_format == SampleFormat::UINT8 ) {
        uint8_t* lines = new uint8_t[tile_height*imageLineSize];

        for ( int y = 0; y < tiles_heightwise; y++ ) {
            // On récupère toutes les lignes pour cette ligne de tuiles
            for (int lig = 0; lig < tile_height; lig++) {
                if (pIn->get_line(lines + lig*imageLineSize, y*tile_height + lig) == 0) {
                    BOOST_LOG_TRIVIAL(error) << "Error reading the source image's line " << y*tile_height + lig;
                    return -1;                    
                }
            }
            for ( int x = 0; x < tiles_widthwise; x++ ) {
                // On constitue la tuile
                for (int lig = 0; lig < tile_height; lig++) {
                    memcpy(tile + lig*raw_tile_line_size, lines + lig*imageLineSize + x*tileLineSize, raw_tile_line_size);
                }
                int tileInd = y*tiles_widthwise + x;

                if (! write_tile(tileInd, tile)) {
                    BOOST_LOG_TRIVIAL(error) << "Error writting tile " << tileInd << " for ROK4 image " << name;
                    return -1;
                }
            }
        }
        
        delete [] lines;
    } else if ( sample_format == SampleFormat::UINT16 ) {
        uint16_t* lines = new uint16_t[tile_height*imageLineSize];
        
        for ( int y = 0; y < tiles_heightwise; y++ ) {
            // On récupère toutes les lignes pour cette ligne de tuiles
            for (int lig = 0; lig < tile_height; lig++) {
                if (pIn->get_line(lines + lig*imageLineSize, y*tile_height + lig) == 0) {
                    BOOST_LOG_TRIVIAL(error) << "Error reading the source image's line " << y*tile_height + lig;
                    return -1;                    
                }
            }
            for ( int x = 0; x < tiles_widthwise; x++ ) {
                // On constitue la tuile
                for (int lig = 0; lig < tile_height; lig++) {
                    memcpy(tile + lig*raw_tile_line_size, lines + lig*imageLineSize + x*tileLineSize, raw_tile_line_size);
                }

                int tileInd = y*tiles_widthwise + x;

                if (! write_tile(tileInd, tile)) {
                    BOOST_LOG_TRIVIAL(error) << "Error writting tile " << tileInd << " for ROK4 image " << name;
                    return -1;
                }
            }
        }
        delete [] lines;
    } else if ( sample_format == SampleFormat::FLOAT32 ) {
        float* lines = new float[tile_height*imageLineSize];
        
        for ( int y = 0; y < tiles_heightwise; y++ ) {
            // On récupère toutes les lignes pour cette ligne de tuiles
            for (int lig = 0; lig < tile_height; lig++) {
                if (pIn->get_line(lines + lig*imageLineSize, y*tile_height + lig) == 0) {
                    BOOST_LOG_TRIVIAL(error) << "Error reading the source image's line " << y*tile_height + lig;
                    return -1;                    
                }
            }
            for ( int x = 0; x < tiles_widthwise; x++ ) {
                // On constitue la tuile
                for (int lig = 0; lig < tile_height; lig++) {
                    memcpy(tile + lig*raw_tile_line_size, lines + lig*imageLineSize + x*tileLineSize, raw_tile_line_size);
                }

                int tileInd = y*tiles_widthwise + x;

                if (! write_tile(tileInd, tile)) {
                    BOOST_LOG_TRIVIAL(error) << "Error writting tile " << tileInd << " for ROK4 image " << name;
                    return -1;
                }
            }
        }
        delete [] lines;
    }
    
    delete [] tile;

    if (! write_final()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot close the ROK4 images (write index) for " << name;
        return -1;
    }

    if (! clear_buffers()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot clean buffers for " << name;
        return -1;
    }
    
    return 0;
}

int Rok4Image::writePbfTiles ( int ulTileCol, int ulTileRow, char* rootDirectory )
{

    if (! is_vector) {
        BOOST_LOG_TRIVIAL(error) << "Write PBF tiles in a slab is possible only for vector ROK4 slabs";
        return -1;
    }

    
    if (! write_header()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot write the ROK4 images header for " << name;
        return -1;
    }

    if (! prepare_buffers()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot initialize buffers for " << name;
        return -1;
    }

    char pbfpath [512];
    for (int row = 0; row < tiles_heightwise; row++) {
        for ( int col = 0; col < tiles_widthwise; col++ ) {
            // Constitution du chemin de la tuile PBF à écrire en l'état dans la dalle
            sprintf (pbfpath, "%s/%d/%d.pbf", rootDirectory, ulTileCol + col, ulTileRow + row);
            BOOST_LOG_TRIVIAL(debug) << "Slabization of pbf tile " << pbfpath;

            if (! write_tile(row * tiles_widthwise + col, pbfpath)) {
                BOOST_LOG_TRIVIAL(error) << "Error writting PBF tile " << pbfpath;
                return -1;
            }
        }   
    }


    if (! write_final()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot close the ROK4 images (write index) for " << name;
        return -1;
    }

    if (! clear_buffers()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot clean buffers for " << name;
        return -1;
    }

    return 0;
}

bool Rok4Image::write_header()
{
    if (! context->open_to_write(name)) {
        BOOST_LOG_TRIVIAL(error) << "Unable to open output " << name;
        return false;
    }

    char header[ROK4_IMAGE_HEADER_SIZE], *p = header;
    memset ( header, 0, sizeof ( header ) );

    * ( ( uint16_t* ) ( p ) )      = 0x4949;    // Little Endian
    * ( ( uint16_t* ) ( p + 2 ) ) = 42;        // Tiff specification
    * ( ( uint32_t* ) ( p + 4 ) ) = 16;        // Offset of the IFD
    p += 8;

    // write the number of entries in the IFD

    if (is_vector) {
        // We can have 4 samples per pixel, each sample with the same size
        * ( ( uint16_t* ) ( p ) ) = (uint16_t) 0;
        * ( ( uint16_t* ) ( p + 2 ) ) = (uint16_t) 0;
        * ( ( uint16_t* ) ( p + 4 ) ) = (uint16_t) 0;
        * ( ( uint16_t* ) ( p + 6 ) ) = (uint16_t) 0;
        p += 8;

        // Number of tags
        * ( ( uint16_t* ) p ) = 4;
        p += 2;

        //  Offset of the IFD is here
        writeTIFFTAG(&p, TIFFTAG_IMAGEWIDTH, TIFF_LONG, 1, width);
        writeTIFFTAG(&p, TIFFTAG_IMAGELENGTH, TIFF_LONG, 1, height);

        if ( tiles_count == 1 ) {
            /* Dans le cas d'une tuile unique, le champs contient directement la valeur et pas l'adresse de la valeur.
             * Cependant, étant donnée le mode de foncionnement de Rok4, on doit laisser la valeur au début de l'image.
             * Voilà pourquoi on ajoute 8 à ROK4_IMAGE_HEADER_SIZE : 4 pour le TileOffset et 4 pour le TileByteCount.
             */
            writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE + 8);
        } else {
            writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE);
        }

        // Dans le cas d'un tuile unique, on viendra écraser la valeur mise ici avec directement sa taille
        writeTIFFTAG(&p, TIFFTAG_TILEBYTECOUNTS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE + 4 * tiles_count);

    } else {
        int bitspersample = SampleFormat::get_bits_per_sample(sample_format);
        // We can have 4 samples per pixel, each sample with the same size
        * ( ( uint16_t* ) ( p ) ) = (uint16_t) bitspersample;
        * ( ( uint16_t* ) ( p + 2 ) ) = (uint16_t) bitspersample;
        * ( ( uint16_t* ) ( p + 4 ) ) = (uint16_t) bitspersample;
        * ( ( uint16_t* ) ( p + 6 ) ) = (uint16_t) bitspersample;
        p += 8;

        // Number of tags
        * ( ( uint16_t* ) p ) = 11;
        if ( photometric == Photometric::YCBCR ) * ( ( uint16_t* ) p ) += 1;
        if ( channels == 4 || channels == 2 ) * ( ( uint16_t* ) p ) += 1;
        p += 2;

        //  Offset of the IFD is here
        writeTIFFTAG(&p, TIFFTAG_IMAGEWIDTH, TIFF_LONG, 1, width);
        writeTIFFTAG(&p, TIFFTAG_IMAGELENGTH, TIFF_LONG, 1, height);

        if ( channels == 1 ) {
            writeTIFFTAG(&p, TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, 1, bitspersample);
        } else if ( channels == 2 ) {
            * ( ( uint16_t* ) ( p ) ) = TIFFTAG_BITSPERSAMPLE;
            * ( ( uint16_t* ) ( p + 2 ) ) = TIFF_SHORT;
            * ( ( uint32_t* ) ( p + 4 ) ) = 2;
            * ( ( uint16_t* ) ( p + 8 ) ) = 8;
            * ( ( uint16_t* ) ( p + 10 ) )  = 8;
            p += 12;
        } else {
            writeTIFFTAG(&p, TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, channels, 8);
        }

        writeTIFFTAG(&p, TIFFTAG_COMPRESSION, TIFF_SHORT, 1, from_rok4_compression(compression));
        writeTIFFTAG(&p, TIFFTAG_PHOTOMETRIC, TIFF_SHORT, 1, from_rok4_photometric(photometric));
        writeTIFFTAG(&p, TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT, 1, channels);
        writeTIFFTAG(&p, TIFFTAG_TILEWIDTH, TIFF_LONG, 1, tile_width);
        writeTIFFTAG(&p, TIFFTAG_TILELENGTH, TIFF_LONG, 1, tile_height);

        if ( tiles_count == 1 ) {
            /* Dans le cas d'une tuile unique, le champs contient directement la valeur et pas l'adresse de la valeur.
             * Cependant, étant donnée le mode de foncionnement de Rok4, on doit laisser la valeur au début de l'image.
             * Voilà pourquoi on ajoute 8 à ROK4_IMAGE_HEADER_SIZE : 4 pour le TileOffset et 4 pour le TileByteCount.
             */
            writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE + 8);
        } else {
            writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE);
        }

        // Dans le cas d'un tuile unique, on vidra écraser la valeur mise ici avec directement sa taille
        writeTIFFTAG(&p, TIFFTAG_TILEBYTECOUNTS, TIFF_LONG, tiles_count, ROK4_IMAGE_HEADER_SIZE + 4 * tiles_count);

        if ( channels == 4 || channels == 2 ) {
            writeTIFFTAG(&p, TIFFTAG_EXTRASAMPLES, TIFF_SHORT, 1, from_rok4_extrasample(extra_sample));
        }

        writeTIFFTAG(&p, TIFFTAG_SAMPLEFORMAT, TIFF_SHORT, 1, from_rok4_sampleformat(sample_format));

        if ( photometric == Photometric::YCBCR ) {
            * ( ( uint16_t* ) ( p ) ) = TIFFTAG_YCBCRSUBSAMPLING;
            * ( ( uint16_t* ) ( p + 2 ) ) = TIFF_SHORT;
            * ( ( uint32_t* ) ( p + 4 ) ) = 2;
            * ( ( uint16_t* ) ( p + 8 ) ) = 2;
            * ( ( uint16_t* ) ( p + 10 ) )  = 2;
            p += 12;
        }
    }

    // end of IFD
    * ( ( uint32_t* ) ( p ) ) = 0;
    p += 4;

    if (context->write((uint8_t*) header, 0, ROK4_IMAGE_HEADER_SIZE, std::string(name)) < 0) {
        return false;
    }

    return true;
}

bool Rok4Image::prepare_buffers()
{

    tiles_offsets = new uint32_t[tiles_count];
    tiles_sizes = new uint32_t[tiles_count];
    memset ( tiles_offsets, 0, tiles_count*4 );
    memset ( tiles_sizes, 0, tiles_count*4 );
    position = ROK4_IMAGE_HEADER_SIZE + 8 * tiles_count;

    if (! is_vector) {
        int quality = 0;
        if ( compression == Compression::PNG) quality = 5;
        if ( compression == Compression::DEFLATE ) quality = 6;
        if ( compression == Compression::JPEG ) quality = 75;
        if ( compression == Compression::JPEG90 ) quality = 90;

        // variables initalizations

        buffer_size = 2*raw_tile_size;
        buffer = new uint8_t[buffer_size];

        //  z compression initalization
        if ( compression == Compression::PNG || compression == Compression::DEFLATE ) {
            if ( compression == Compression::PNG ) {
                // Pour la compression PNG, on a besoin d'un octet par ligne ne plus : un 0 est ajouté au début de chaque ligne, avant la compression
                zip_buffer = new uint8_t[raw_tile_size + tile_height];
            } else {
                zip_buffer = new uint8_t[raw_tile_size];            
            }
            zstream.zalloc = Z_NULL;
            zstream.zfree  = Z_NULL;
            zstream.opaque = Z_NULL;
            zstream.data_type = Z_BINARY;
            deflateInit ( &zstream, quality );
        }

        if ( compression == Compression::JPEG || compression == Compression::JPEG90 ) {
            cinfo.err = jpeg_std_error ( &jerr );
            jpeg_create_compress ( &cinfo );

            cinfo.dest = new jpeg_destination_mgr;
            cinfo.dest->init_destination = init_destination;
            cinfo.dest->empty_output_buffer = empty_output_buffer;
            cinfo.dest->term_destination = term_destination;

            cinfo.image_width  = tile_width;
            cinfo.image_height = tile_height;
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;

            jpeg_set_defaults ( &cinfo );
            jpeg_set_quality ( &cinfo, quality, true );
        }
    }

    return true;
}


bool Rok4Image::write_final() {
    context->write((uint8_t*) tiles_offsets, ROK4_IMAGE_HEADER_SIZE, 4 * tiles_count, std::string(name));
    context->write((uint8_t*) tiles_sizes, ROK4_IMAGE_HEADER_SIZE + 4 * tiles_count, 4 * tiles_count, std::string(name));

    if (! context->close_to_write(name)) {
        BOOST_LOG_TRIVIAL(error) << "Unable to close output " << name;
        return false;
    }

    return true;
}

bool Rok4Image::clear_buffers() {

    if (! is_vector) {
        delete[] buffer;
        if ( compression == Compression::PNG || compression == Compression::DEFLATE ) {
            delete[] zip_buffer;
            deflateEnd ( &zstream );
        }
        if ( compression == Compression::JPEG ) {
            delete cinfo.dest;
            jpeg_destroy_compress ( &cinfo );
        }
    }

    return true;
}

// Raster write tile in a slab
bool Rok4Image::write_tile( int tileInd, uint8_t* data)
{
    
    if ( tileInd > tiles_count || tileInd < 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unvalid tile's indice to write (" << tileInd << "). Have to be between 0 and " << tiles_count-1 ;
        return false;
    }

    size_t size;


    switch ( compression ) {
    case Compression::NONE:
        size = compute_raw_tile ( buffer, data );
        break;
    case Compression::LZW :
        size = compute_lzw_tile ( buffer, data );
        break;
    case Compression::JPEG:
        size = compute_jpeg_tile ( buffer, data );
        break;
    case Compression::JPEG90:
        size = compute_jpeg_tile ( buffer, data );
        break;
    case Compression::PNG :
        size = compute_png_tile ( buffer, data );
        break;
    case Compression::PACKBITS :
        size = compute_pkb_tile ( buffer, data );
        break;
    case Compression::DEFLATE :
        size = compute_zip_tile ( buffer, data );
        break;
    }

    if ( size == 0 ) return false;

    if ( tiles_count == 1 ) {

        uint8_t* uint32tab = new uint8_t[sizeof( uint32_t )];
        *((uint32_t*) uint32tab) = ( uint32_t ) size;
        context->write(uint32tab, 134, 4, std::string(name));
        delete uint32tab;

    }

    tiles_offsets[tileInd] = position;
    tiles_sizes[tileInd] = size;

    boolean ret = context->write(buffer, position, size, std::string(name));

    if (! ret) {
        BOOST_LOG_TRIVIAL(error) << "Impossible to write the tile " << tileInd;
        return false;
    }
    position = ( position + size + 15 ) & ~15; // Align the next position on 16byte

    return true;
}

// Vector write tile in a slab
bool Rok4Image::write_tile( int tileInd, char* pbfpath )
{
    
    if ( tileInd > tiles_count || tileInd < 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unvalid tile's indice to write (" << tileInd << "). Have to be between 0 and " << tiles_count-1 ;
        return false;
    }

    std::ifstream::pos_type data_size;
    std::vector<char> data;
    std::ifstream ifs(pbfpath, std::ios::binary|std::ios::ate);

    if (! ifs.is_open()) {
        BOOST_LOG_TRIVIAL(debug) << "Cannot open PBF tile " << pbfpath;
        data_size = 0;
    } else {

        data_size = ifs.tellg();

        if (ifs.bad()) {
            BOOST_LOG_TRIVIAL(error) << "Error reading size fo PBF tile " << pbfpath;
            return false;
        }

        data.resize(data_size);

        ifs.seekg(0, std::ios::beg);
        ifs.read(data.data(), data_size);

        if (ifs.bad()) {
            BOOST_LOG_TRIVIAL(error) << "Error reading PBF tile " << pbfpath;
            return false;
        }

        ifs.close();

        if ( data_size == 0 ) return false;
    }

    if ( tiles_count == 1 ) {

        uint8_t* uint32tab = new uint8_t[sizeof( uint32_t )];
        *((uint32_t*) uint32tab) = ( uint32_t ) data_size;
        context->write(uint32tab, 134, 4, std::string(name));
        delete uint32tab;

    }

    tiles_offsets[tileInd] = position;
    tiles_sizes[tileInd] = data_size;

    boolean ret = context->write((uint8_t*) data.data(), position, data_size, std::string(name));

    if (! ret) {
        BOOST_LOG_TRIVIAL(error) << "Impossible to write the tile " << tileInd;
        return false;
    }
    position = ( position + data_size + 15 ) & ~15; // Align the next position on 16byte

    return true;
}

size_t Rok4Image::compute_raw_tile ( uint8_t *buffer, uint8_t *data ) {
    memcpy ( buffer, data, raw_tile_size );
    return raw_tile_size;
}

size_t Rok4Image::compute_lzw_tile ( uint8_t *buffer, uint8_t *data ) {

    size_t outSize;

    LzwCompressor LZWE;
    uint8_t* temp = LZWE.encode ( data, raw_tile_size, outSize );

    if ( outSize > buffer_size ) {
        delete[] buffer;
        buffer_size = outSize * 2;
        buffer = new uint8_t[buffer_size];
    }
    memcpy ( buffer,temp,outSize );
    delete [] temp;

    return outSize;
}

size_t Rok4Image::compute_pkb_tile ( uint8_t *buffer, uint8_t *data ) {

    uint8_t* pkbBuffer = new uint8_t[raw_tile_line_size*tile_height*2];
    size_t pkbBufferSize = 0;
    uint8_t* rawLine = new uint8_t[raw_tile_line_size];
    int lRead = 0;
    PkbCompressor encoder;
    uint8_t * pkbLine;
    for ( ; lRead < tile_height ; lRead++ ) {
        memcpy ( rawLine,data+lRead*raw_tile_line_size,raw_tile_line_size );
        size_t pkbLineSize = 0;
        pkbLine = encoder.encode ( rawLine, raw_tile_line_size, pkbLineSize );
        memcpy ( pkbBuffer+pkbBufferSize,pkbLine,pkbLineSize );
        pkbBufferSize += pkbLineSize;
        delete[] pkbLine;
    }

    memcpy ( buffer,pkbBuffer,pkbBufferSize );
    delete[] pkbBuffer;
    delete[] rawLine;

    return pkbBufferSize;
}

size_t Rok4Image::compute_png_tile ( uint8_t *buffer, uint8_t *data ) {
    uint8_t *B = zip_buffer;
    for ( unsigned int h = 0; h < tile_height; h++ ) {
        *B++ = 0; // on met un 0 devant chaque ligne (spec png -> mode de filtrage simple)
        memcpy ( B, data + h*raw_tile_line_size, raw_tile_line_size );
        B += raw_tile_line_size;
    }

    memcpy ( buffer, PNG_HEADER, sizeof ( PNG_HEADER ) );
    * ( ( uint32_t* ) ( buffer+16 ) ) = bswap_32 ( tile_width );
    * ( ( uint32_t* ) ( buffer+20 ) ) = bswap_32 ( tile_height );
    if ( channels == 1 ) {
        buffer[25] = 0;    // GRAY
    } else if ( channels == 2 ) {
        buffer[25] = 4;    // GRAYA
    }else if ( channels == 3 ) {
        buffer[25] = 2;    // RGB
    } else if ( channels == 4 ) {
        buffer[25] = 6;    // RGBA
    }

    uint32_t crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + 12, 17 );
    * ( ( uint32_t* ) ( buffer+29 ) ) = bswap_32 ( crc );

    zstream.next_out  = buffer + sizeof ( PNG_HEADER ) + 8;
    zstream.avail_out = 2*raw_tile_size - 12 - sizeof ( PNG_HEADER ) - sizeof ( PNG_IEND );
    zstream.next_in   = zip_buffer;
    zstream.avail_in  = raw_tile_size + tile_height;

    if ( deflateReset ( &zstream ) != Z_OK ) return -1;
    if ( deflate ( &zstream, Z_FINISH ) != Z_STREAM_END ) return -1;

    * ( ( uint32_t* ) ( buffer+sizeof ( PNG_HEADER ) ) ) =  bswap_32 ( zstream.total_out );
    buffer[sizeof ( PNG_HEADER ) + 4] = 'I';
    buffer[sizeof ( PNG_HEADER ) + 5] = 'D';
    buffer[sizeof ( PNG_HEADER ) + 6] = 'A';
    buffer[sizeof ( PNG_HEADER ) + 7] = 'T';

    crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + sizeof ( PNG_HEADER ) + 4, zstream.total_out+4 );
    * ( ( uint32_t* ) zstream.next_out ) = bswap_32 ( crc );

    memcpy ( zstream.next_out + 4, PNG_IEND, sizeof ( PNG_IEND ) );
    return zstream.total_out + 12 + sizeof ( PNG_IEND ) + sizeof ( PNG_HEADER );
}

size_t Rok4Image::compute_zip_tile ( uint8_t *buffer, uint8_t *data ) {
    uint8_t *B = zip_buffer;
    for ( unsigned int h = 0; h < tile_height; h++ ) {
        memcpy ( B, data + h*raw_tile_line_size, raw_tile_line_size );
        B += raw_tile_line_size;
    }
    zstream.next_out  = buffer;
    zstream.avail_out = 2*raw_tile_size;
    zstream.next_in   = zip_buffer;
    zstream.avail_in  = raw_tile_size;

    if ( deflateReset ( &zstream ) != Z_OK ) return -1;
    if ( deflate ( &zstream, Z_FINISH ) != Z_STREAM_END ) return -1;
    
    return zstream.total_out;
}


size_t Rok4Image::compute_jpeg_tile ( uint8_t *buffer, uint8_t *data ) {

    cinfo.dest->next_output_byte = buffer;
    cinfo.dest->free_in_buffer = 2*raw_tile_size;
    jpeg_start_compress ( &cinfo, true );

    int numLine = 0;

    while ( numLine < tile_height ) {

        uint8_t* line = data + numLine*raw_tile_line_size;

        if ( jpeg_write_scanlines ( &cinfo, &line, 1 ) != 1 ) return 0;
        numLine++;
    }

    jpeg_finish_compress ( &cinfo );

    return 2*raw_tile_size - cinfo.dest->free_in_buffer;
}


