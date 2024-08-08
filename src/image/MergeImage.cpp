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
 * \file MergeImage.cpp
 ** \~french
 * \brief Implémentation des classes MergeImage et MergeMask et du namespace Merge
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 * \li Merge : énumère et manipule les différentes méthodes de fusion
 ** \~english
 * \brief Implement classes MergeImage and MergeMask and the namespace Merge
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeMask : merged mask, associated with a merged image
 * \li Merge : enumerate and managed different merge methods
 */

#include "image/MergeImage.h"

#include "utils/Utils.h"
#include <boost/log/trivial.hpp>
#include <cstring>
#include "processors/Line.h"

template <typename tBuf>
int MergeImage::_getline ( tBuf* buffer, int line ) {
    Line aboveLine ( width, sizeof(tBuf) );
    tBuf* imageLine = new tBuf[width*4];
    uint8_t* maskLine = new uint8_t[width];
    memset ( maskLine, 0, width );

    tBuf bg[channels*width];
    for ( int i = 0; i < channels*width; i++ ) {
        bg[i] = ( tBuf ) bgValue[i%channels];
    }
    Line workLine ( bg, maskLine, channels, width );

    tBuf* transparent;
    if ( transparentValue != NULL ) {
        transparent = new tBuf[3];
        for ( int i = 0; i < 3; i++ ) {
            transparent[i] = ( tBuf ) transparentValue[i];
        }
    }

    for ( int i = 0; i < images.size(); i++ ) {

        int srcSpp = images[i]->get_channels();
        images[i]->get_line ( imageLine,line );

        if ( images[i]->get_mask() == NULL ) {
            memset ( maskLine, 255, width );
        } else {
            images[i]->get_mask()->get_line ( maskLine,line );
        }

        if ( transparentValue == NULL ) {
            aboveLine.store ( imageLine, maskLine, srcSpp );
        } else {
            aboveLine.store ( imageLine, maskLine, srcSpp, transparent );
        }

        switch ( composition ) {
        case Merge::NORMAL:
            workLine.use_masks ( &aboveLine );
            break;
        case Merge::TOP:
            workLine.use_masks ( &aboveLine );
            break;
        case Merge::MULTIPLY:
            workLine.multiply ( &aboveLine );
            break;
        case Merge::ALPHATOP:
            workLine.alphaBlending ( &aboveLine );
            break;
            //case Merge::LIGHTEN:
            //case Merge::DARKEN:
        default:
            workLine.use_masks ( &aboveLine );
            break;
        }

    }

    // On repasse la ligne sur le nombre de canaux voulu
    workLine.write ( buffer, channels );

    if ( transparentValue != NULL ) {
        delete [] transparent;
    }
    delete [] imageLine;
    delete [] maskLine;

    return width*channels*sizeof( tBuf );
}

/* Implementation de get_line pour les uint8_t */
int MergeImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les uint8_t */
int MergeImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int MergeImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

MergeImage* MergeImage::create ( std::vector< Image* >& images, int channels,
        int* bgValue, int* transparentValue, Merge::eMergeType composition ) {
    if ( images.size() == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "No source images to defined merged image" ;
        return NULL;
    }

    int width = images.at ( 0 )->get_width();
    int height = images.at ( 0 )->get_height();

    for ( int i = 1; i < images.size(); i++ ) {
        if ( images.at ( i )->get_width() != width || images.at ( i )->get_height() != height ) {
            BOOST_LOG_TRIVIAL(error) <<  "All images must have same dimensions" ;
            images.at ( 0 )->print();
            images.at ( i )->print();
            return NULL;
        }
    }

    if ( bgValue == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "We have to precise a value used as background in the MergeImage" ;
        return NULL;
    }

    return new MergeImage ( images, channels, bgValue, transparentValue, composition );
}

/* Implementation de get_line pour les uint8_t */
int MergeMask::get_line ( uint8_t* buffer, int line ) {
    memset ( buffer,0,width );

    uint8_t* buffer_m = new uint8_t[width];

    for ( uint i = 0; i < MI->get_images()->size(); i++ ) {

        if ( MI->get_mask ( i ) == NULL ) {
            /* L'image n'a pas de masque, on la considère comme pleine. Ca ne sert à rien d'aller voir plus loin,
             * cette ligne du masque est déjà pleine */
            memset ( buffer, 255, width );
            delete [] buffer_m;
            return width;
        } else {
            // Récupération du masque de l'image courante de l'MI.
            MI->get_mask ( i )->get_line ( buffer_m,line );
            // On ajoute au masque actuel (on écrase si la valeur est différente de 0)
            for ( int j = 0; j < width; j++ ) {
                if ( buffer_m[j] ) {
                    memcpy ( &buffer[j],&buffer_m[j],1 );
                }
            }
        }
    }

    delete [] buffer_m;
    return width;
}

/* Implementation de get_line pour les uint16 */
int MergeMask::get_line ( uint16_t* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = get_line ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

/* Implementation de get_line pour les float */
int MergeMask::get_line ( float* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = get_line ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

namespace Merge {

const char *mergeType_name[] = {
    "UNKNOWN",
    "NORMAL",
    "LIGHTEN",
    "DARKEN",
    "MULTIPLY",
    "ALPHATOP",
    "TOP"
};

eMergeType from_string ( std::string strMergeMethod ) {
    int i;
    for ( i = mergeType_size; i ; --i ) {
        if ( strMergeMethod.compare ( mergeType_name[i] ) == 0 )
            break;
    }
    return static_cast<eMergeType> ( i );
}

std::string to_string ( eMergeType merge_method ) {
    return std::string ( mergeType_name[merge_method] );
}
}
