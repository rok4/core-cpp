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
 * \file BilzImage.cpp
 ** \~french
 * \brief Implémentation des classes BilzImage
 * \details
 * \li BilzImage : gestion d'une image au format PNG, en lecture
 ** \~english
 * \brief Implement classes BilzImage
 * \details
 * \li BilzImage : manage a (Z)BIL format image, reading
 */

#include "image/file/BilzImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
BilzImage* BilzImage::create_to_read ( std::string filename, BoundingBox< double > bbox, double resx, double resy ) {
        
    
    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    Compression::eCompression comp = Compression::NONE;
    
    if (! readHeaderFile(filename, &width, &height, &channels, &bitspersample)) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot read header associated to (Z)Bil image " << filename ;
        return NULL;        
    }
    
    if (width == 0 || height == 0 || channels == 0 || bitspersample == 0) {
        BOOST_LOG_TRIVIAL(error) <<  "Missing or invalid information in the header associated to (Z)Bil image " << filename ;
        return NULL;         
    }
    
    switch (bitspersample) {
        case 32 :
            sf = SampleFormat::FLOAT32;
            break;
        case 16 :
            sf = SampleFormat::UINT16;
            break;
        case 8 :
            sf = SampleFormat::UINT8;
            break;
        default :
            BOOST_LOG_TRIVIAL(error) <<  "Unhandled number of bits per sample (" << bitspersample << ") for image " << filename ;
            return NULL;
    }
    
    switch (channels) {
        case 1 :
        case 2 :
            ph = Photometric::GRAY;
            break;
        case 3 :
        case 4 :
            ph = Photometric::RGB;
            break;
        default :
            BOOST_LOG_TRIVIAL(error) <<  "Unhandled number of samples pixel (" << channels << ") for image " << filename ;
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
    
    FILE *file = fopen ( filename.c_str(), "rb" );
    if ( ! file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open the file (to read) " << filename ;
        return NULL;
    }
    
    int rawdatasize = width * height * channels * bitspersample / 8;
    uint8_t* bilData = new uint8_t[rawdatasize];
    
    size_t readdatasize = fread ( bilData, 1, rawdatasize, file );
    
    fclose ( file );

    if (readdatasize < rawdatasize ) {
        BOOST_LOG_TRIVIAL(debug) << "Data in bil image are compressed (smaller than expected). We try to uncompressed them (deflate).";
        comp = Compression::DEFLATE;
        
        uint8_t* tmpData = new uint8_t[readdatasize];
        memcpy(tmpData, bilData, readdatasize);
        
        if (! uncompressedData(tmpData, readdatasize, bilData, rawdatasize)) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot uncompressed data for file " << filename ;
            BOOST_LOG_TRIVIAL(error) <<  "Only deflate compression is supported";
            return NULL;
        }
        
        delete [] tmpData;
    }
    
    
    
    /******************** CRÉATION DE L'OBJET ******************/
    
    return new BilzImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, ph, comp,
        bilData
    );
    
}


/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

BilzImage::BilzImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric, Compression::eCompression compression,
    uint8_t* bilData ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sample_format, photometric, compression ),

    data(bilData) {
        
    //tmpbuffer = new uint8_t[width * pixel_size];
    
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int BilzImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];
    
    // Si on a un seul canal, il n'y a rien à faire (simple copie depuis le buffer data). Pour gagner du temps, on le teste
    if (channels == 1) {
        memcpy(buffertmp, data + line * width * pixel_size, width * pixel_size);
    } else {
    
        int samplesize = sizeof(T);
        
        for (int s = 0; s < channels; s++) {
            uint8_t* deb = data + line * width * pixel_size;
            for (int p = 0; p < width; p++) {
                memcpy(buffertmp + p * pixel_size + s * samplesize, deb + p * samplesize, samplesize );
            }
        }
    }

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convert_line(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixel_size * width);
    }
    
    return width * get_channels();
}

int BilzImage::get_line ( uint8_t* buffer, int line ) {
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

int BilzImage::get_line ( uint16_t* buffer, int line ) {
    
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

int BilzImage::get_line ( float* buffer, int line ) {
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

