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
 * \file MirrorImage.cpp
 ** \~french
 * \brief Implémentation des classe MirrorImage
 * \details
 * \li MirrorImage : image par reflet
 ** \~english
 * \brief Implement classes MirrorImage
 * \details
 * \li MirrorImage : reflection image
 */

#include <boost/log/trivial.hpp>

#include "image/MirrorImage.h"
#include "utils/Utils.h"

MirrorImage* MirrorImage::create ( Image* pImageSrc, int position, uint mirrorSize ) {

    int wTopBottom = pImageSrc->get_width() +2*mirrorSize;
    int wLeftRight = mirrorSize;
    int hTopBottom = mirrorSize;
    int hLeftRight = pImageSrc->get_height();

    double xmin,ymin,xmax,ymax;

    if ( pImageSrc == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Source image is NULL" ;
        return NULL;
    }

    if ( mirrorSize > pImageSrc->get_width() || mirrorSize > pImageSrc->get_height() ) {
        BOOST_LOG_TRIVIAL(error) <<  "Image is smaller than what we need for mirrors (we need "<< mirrorSize << " pixels)" ;
        return NULL;
    }

    if ( position == 0 ) {
        // TOP
        xmin=pImageSrc->get_xmin()-pImageSrc->get_resx() *mirrorSize;
        xmax=pImageSrc->get_xmax() +pImageSrc->get_resx() *mirrorSize;
        ymin=pImageSrc->get_ymax();
        ymax=pImageSrc->get_ymax() +pImageSrc->get_resy() *mirrorSize;
        BoundingBox<double> bbox ( xmin,ymin,xmax,ymax );
        return new MirrorImage ( wTopBottom,hTopBottom,pImageSrc->get_channels(),bbox,pImageSrc,0,mirrorSize );
    } else if ( position == 1 ) {
        // RIGHT
        xmin=pImageSrc->get_xmax();
        xmax=pImageSrc->get_xmax() +pImageSrc->get_resx() *mirrorSize;
        ymin=pImageSrc->get_ymin();
        ymax=pImageSrc->get_ymax();
        BoundingBox<double> bbox ( xmin,ymin,xmax,ymax );
        return new MirrorImage ( wLeftRight,hLeftRight,pImageSrc->get_channels(),bbox,pImageSrc,1,mirrorSize );
    } else if ( position == 2 ) {
        // BOTTOM
        xmin=pImageSrc->get_xmin()-pImageSrc->get_resx() *mirrorSize;
        xmax=pImageSrc->get_xmax() +pImageSrc->get_resx() *mirrorSize;
        ymin=pImageSrc->get_ymin()-pImageSrc->get_resy() *mirrorSize;
        ymax=pImageSrc->get_ymin();
        BoundingBox<double> bbox ( xmin,ymin,xmax,ymax );
        return new MirrorImage ( wTopBottom,hTopBottom,pImageSrc->get_channels(),bbox,pImageSrc,2,mirrorSize );
    } else if ( position == 3 ) {
        // LEFT
        xmin=pImageSrc->get_xmin()-pImageSrc->get_resx() *mirrorSize;
        xmax=pImageSrc->get_xmin();
        ymin=pImageSrc->get_ymin();
        ymax=pImageSrc->get_ymax();
        BoundingBox<double> bbox ( xmin,ymin,xmax,ymax );
        return new MirrorImage ( wLeftRight,hLeftRight,pImageSrc->get_channels(),bbox,pImageSrc,3,mirrorSize );
    } else {
        return NULL;
    }

}

template <typename T>
int MirrorImage::_getline ( T* buffer, int line ) {
    uint32_t line_size=width*channels;
    T* buf0=new T[source_image->get_width() *channels];


    if ( position == 0 ) {
        // TOP
        int lineSrc = height - line -1;

        source_image->get_line ( buf0,lineSrc );

        memcpy ( &buffer[size*channels],buf0,source_image->get_width() *channels*sizeof ( T ) );
        for ( int j = 0; j < size; j++ ) {
            memcpy ( &buffer[j*channels],&buf0[ ( size-j-1 ) *channels],channels*sizeof ( T ) ); // left
            memcpy ( &buffer[ ( width-j-1 ) *channels],&buf0[ ( source_image->get_width() - size + j ) *channels],channels*sizeof ( T ) ); // right
        }

    } else if ( position == 1 ) {
        // RIGHT
        source_image->get_line ( buf0,line );
        for ( int j = 0; j < size; j++ ) {
            memcpy ( &buffer[j*channels],&buf0[ ( source_image->get_width()-j-1 ) *channels],channels*sizeof ( T ) ); // right
        }

    } else if ( position == 2 ) {
        // BOTTOM
        int lineSrc = source_image->get_height() - line -1;

        source_image->get_line ( buf0,lineSrc );

        memcpy ( &buffer[size*channels],buf0,source_image->get_width() *channels*sizeof ( T ) );
        for ( int j = 0; j < size; j++ ) {
            memcpy ( &buffer[j*channels],&buf0[ ( size-j-1 ) *channels],channels*sizeof ( T ) ); // left
            memcpy ( &buffer[ ( width-j-1 ) *channels],&buf0[ ( source_image->get_width() - size + j ) *channels],channels*sizeof ( T ) ); // right
        }

    } else if ( position == 3 ) {
        // LEFT
        source_image->get_line ( buf0,line );
        for ( int j = 0; j < size; j++ ) {
            memcpy ( &buffer[j*channels],&buf0[ ( size-j-1 ) *channels],channels*sizeof ( T ) ); // right
        }

    }

    delete [] buf0;

    return width*channels;
}

/* Implementation de get_line pour les uint8_t */
int MirrorImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les uint16_t */
int MirrorImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int MirrorImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}
