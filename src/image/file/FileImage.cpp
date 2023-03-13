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
 * \file FileImage.cpp
 ** \~french
 * \brief Implémentation des classes FileImage et FileImageFactory
 * \details
 * \li FileImage : image physique, attaché à un fichier
 * \li FileImageFactory : usine de création d'objet FileImage
 ** \~english
 * \brief Implement classes FileImage and FileImageFactory
 * \details
 * \li FileImage : physical image, linked to a file
 * \li FileImageFactory : factory to create FileImage object
 */

#include "image/file/FileImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"
#include "image/file/LibtiffImage.h"
#include "image/file/LibpngImage.h"
#include "image/file/LibjpegImage.h"
#include "image/file/BilzImage.h"
#ifdef KDU_ENABLED
#include "image/file/kakadu/LibkakaduImage.h"
#else
#include "image/file/openjpeg/LibopenjpegImage.h"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
FileImage* FileImageFactory::createImageToRead ( std::string name, BoundingBox< double > bbox, double resx, double resy ) {

    // Récupération de l'extension du fichier
    const char * pch;
    pch = strrchr ( name.c_str(),'.' );

    if (pch == NULL) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot find the dot to determine extension and driver: " << name ;
        return NULL;
    }

    /********************* TIFF *********************/
    if ( strncmp ( pch+1, "tif", 3 ) == 0 || strncmp ( pch+1, "TIF", 3 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "TIFF image to read : " << name ;

        LibtiffImageFactory LTIF;
        return LTIF.createLibtiffImageToRead ( name, bbox, resx, resy );
    }

    // Les masques
    else if ( strncmp ( pch+1, "msk", 3 ) == 0 || strncmp ( pch+1, "MSK", 3 ) == 0 ) {
        /** \~french \warning Les masques sources (fichiers avec l'extension .msk) seront lus comme des images TIFF. */
        BOOST_LOG_TRIVIAL(debug) <<  "TIFF mask to read : " << name ;

        LibtiffImageFactory LTIF;
        return LTIF.createLibtiffImageToRead ( name, bbox, resx, resy );
    }
    
    /******************** (Z)BIL ********************/
    else if ( strncmp ( pch+1, "bil", 3 ) == 0 || strncmp ( pch+1, "BIL", 3 ) == 0) {
        BOOST_LOG_TRIVIAL(debug) <<  "(Z)BIL image to read : " << name ;

        BilzImageFactory BZIF;
        return BZIF.createBilzImageToRead ( name, bbox, resx, resy );
    }
    
    else if ( strncmp ( pch+1, "zbil", 4 ) == 0 || strncmp ( pch+1, "ZBIL", 4 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "(Z)BIL image to read : " << name ;

        BilzImageFactory BZIF;
        return BZIF.createBilzImageToRead ( name, bbox, resx, resy );
    }

    /********************* PNG **********************/
    else if ( strncmp ( pch+1, "png", 3 ) == 0 || strncmp ( pch+1, "PNG", 3 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "PNG image to read : " << name ;

        LibpngImageFactory LPIF;
        return LPIF.createLibpngImageToRead ( name, bbox, resx, resy );
    }

    /********************** JPEG ********************/
    else if ( strncmp ( pch+1, "jpg", 3 ) == 0 || strncmp ( pch+1, "jpeg", 4 ) == 0 || strncmp ( pch+1, "JPEG", 4 ) == 0 || strncmp ( pch+1, "JPG", 3 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "JPEG image to read : " << name ;
        
        LibjpegImageFactory JPGIF;
        return JPGIF.createLibjpegImageToRead ( name, bbox, resx, resy );
    }

    /******************* JPEG 2000 ******************/
    else if ( strncmp ( pch+1, "jp2", 3 ) == 0 || strncmp ( pch+1, "JP2", 3 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "JPEG2000 image to read : " << name ;
        
#ifdef KDU_ENABLED

        BOOST_LOG_TRIVIAL(debug) << "\tDriver : KAKADU";
        BOOST_LOG_TRIVIAL(debug) << "\tThreading : " << KDU_THREADING;

        LibkakaduImageFactory DRVKDU;
        return DRVKDU.createLibkakaduImageToRead(name, bbox, resx, resy);

#else

        BOOST_LOG_TRIVIAL(debug) << "\tDriver : OPENJPEG";

        LibopenjpegImageFactory DRVOJ;
        return DRVOJ.createLibopenjpegImageToRead(name, bbox, resx, resy);
    
#endif
    }

    /* /!\ Format inconnu en lecture /!\ */
    else {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled image's extension (" << pch+1 << "), in the file to read : " << name ;
        return NULL;
    }

}

/* ----- Pour l'écriture ----- */
FileImage* FileImageFactory::createImageToWrite (
    std::string name, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression ) {

    // Récupération de l'extension du fichier
    const char * pch;
    pch = strrchr ( name.c_str(),'.' );

    /********************* TIFF *********************/
    if ( strncmp ( pch+1, "tif", 3 ) == 0 || strncmp ( pch+1, "TIF", 3 ) == 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "TIFF image to write : " << name ;

        LibtiffImageFactory LTIF;
        return LTIF.createLibtiffImageToWrite (
            name, bbox, resx, resy, width, height, channels,
            sampleformat, bitspersample, photometric, compression, 16
        );
    }
    
    // Les masques
    else if ( strncmp ( pch+1, "msk", 3 ) == 0 || strncmp ( pch+1, "MSK", 3 ) == 0 ) {
        /** \~french \warning Les masques sources (fichiers avec l'extension .msk) seront écris comme des images TIFF. */
        BOOST_LOG_TRIVIAL(debug) <<  "TIFF mask to write : " << name ;

        LibtiffImageFactory LTIF;
        return LTIF.createLibtiffImageToWrite (
            name, bbox, resx, resy, width, height, channels,
            sampleformat, bitspersample, photometric, compression, 16
        );
    }

    /* /!\ Format inconnu en écriture /!\ */
    else {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled image's extension (" << pch+1 << "), in the file to write : " << name ;
        return NULL;
    }

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

FileImage::FileImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, std::string name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression, ExtraSample::eExtraSample esType ) :

    Image ( width,height,channels,resx,resy,bbox ),
    sampleformat ( sampleformat ), bitspersample ( bitspersample ), photometric ( photometric ), compression ( compression ),
    esType(esType), filename(name) {

    pixelSize = bitspersample * channels / 8;
    converter = NULL;
}

