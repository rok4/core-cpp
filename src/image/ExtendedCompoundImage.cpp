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
 * \file ExtendedCompoundImage.cpp
 ** \~french
 * \brief Implémentation des classes ExtendedCompoundImage, ExtendedCompoundMask
 * \details
 * \li ExtendedCompoundImage : image composée d'images compatibles, superposables
 * \li ExtendedCompoundMask : masque composé, associé à une image composée
 ** \~english
 * \brief Implement classes ExtendedCompoundImage, ExtendedCompoundMask
 * \details
 * \li ExtendedCompoundImage : image compounded with superimpose images
 * \li ExtendedCompoundMask : compounded mask, associated with a compounded image
 */

#include "image/ExtendedCompoundImage.h"
#include <boost/log/trivial.hpp>
#include "utils/Utils.h"
#include "image/EmptyImage.h"
#include "image/MirrorImage.h"

/********************************************** ExtendedCompoundImage ************************************************/

template <typename T>
int ExtendedCompoundImage::_getline ( T* buffer, int line ) {
    int i;

    // Initialisation de tous les pixels de la ligne avec la valeur de nodata
    for ( i = 0; i < width * channels; i++ ) {
        buffer[i] = ( T ) nodata_value[i%channels];
    }

    for ( i = 0; i < ( int ) source_images.size(); i++ ) {
        
        int lineInSource = line - rows_offsets[i];
        
        // On ecarte les images qui ne se trouvent pas sur la ligne
        // On evite de comparer des coordonnees terrain (comparaison de flottants)
        // Les coordonnees image sont obtenues en arrondissant au pixel le plus proche

        if ( lineInSource < 0 || lineInSource >= source_images[i]->get_height() ) {
            continue;
        }
        if ( source_images[i]->get_xmin() >= get_xmax() || source_images[i]->get_xmax() <= get_xmin() ) {
            continue;
        }

        // c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c0 = c0s[i];
        // c1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c1 = c1s[i];

        // c2 : indice de de la 1ere colonne de l'ExtendedCompoundImage dans l'image courante
        int c2 = c2s[i];

        T* buffer_t = new T[source_images[i]->get_width() * source_images[i]->get_channels()];

        source_images[i]->get_line ( buffer_t,lineInSource );

        if ( get_mask ( i ) == NULL ) {
            memcpy ( &buffer[c0*channels], &buffer_t[c2*channels], ( c1 + 1 - c0) *channels*sizeof ( T ) );
        } else {

            uint8_t* buffer_m = new uint8_t[get_mask ( i )->get_width()];
            get_mask ( i )->get_line ( buffer_m,lineInSource );

            for ( int j=0; j < c1 - c0 + 1; j++ ) {
                if ( buffer_m[c2+j] ) {
                    memcpy ( &buffer[ ( c0 + j ) *channels], &buffer_t[ ( c2+j ) *channels],sizeof ( T ) *channels );
                }
            }

            delete [] buffer_m;
        }
        delete [] buffer_t;
    }
    return width * channels * sizeof ( T );
}


/* Implementation de get_line pour les uint8_t */
int ExtendedCompoundImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les uint16_t */
int ExtendedCompoundImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int ExtendedCompoundImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

bool ExtendedCompoundImage::add_mirrors ( int mirrorSize ) {
    std::vector< Image*>  mirrorImages;

    if ( mirrorSize <= 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to add mirror : mirror's size negative or null : " << mirrorSize ;
        return false;
    }

    if ( source_images.size() == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to add mirror : no source image" ;
        return false;
    }

    for ( uint i = 0; i < source_images.size(); i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            MirrorImage* mirrorImage = MirrorImage::create ( source_images.at ( i ), j, mirrorSize );
            if ( mirrorImage == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  "Unable to calculate image's mirror" ;
                return false;
            }

            if ( source_images.at ( i )->get_mask() ) {
                MirrorImage* mirrorMask = MirrorImage::create ( source_images.at ( i )->get_mask(), j, mirrorSize );
                if ( mirrorMask == NULL ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unable to calculate mask's mirror" ;
                    return false;
                }
                if ( ! mirrorImage->set_mask ( mirrorMask ) ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unable to add mask to mirror" ;
                    return false;
                }
            }

            mirrorImages.push_back ( mirrorImage );
        }
    }

    source_images.insert ( source_images.begin(),mirrorImages.begin(),mirrorImages.end() );
    mirrors_count = mirrorImages.size();

    // Mise à jour des dimensions en tenant compte des miroirs : BBOX et tailles pixel
    for ( unsigned int j = 0; j < mirrors_count; j++ ) {
        if ( source_images.at ( j )->get_xmin() < bbox.xmin )  bbox.xmin = source_images.at ( j )->get_xmin();
        if ( source_images.at ( j )->get_ymin() < bbox.ymin )  bbox.ymin = source_images.at ( j )->get_ymin();
        if ( source_images.at ( j )->get_xmax() > bbox.xmax )  bbox.xmax = source_images.at ( j )->get_xmax();
        if ( source_images.at ( j )->get_ymax() > bbox.ymax )  bbox.ymax = source_images.at ( j )->get_ymax();
    }

    width = int ( ( bbox.xmax-bbox.xmin ) / get_resx() + 0.5 );
    height = int ( ( bbox.ymax-bbox.ymin ) / get_resy() + 0.5 );

    if ( mask ) {
        // Mise à jour du masque associé à l'image composée
        ExtendedCompoundMask* newMask = new ExtendedCompoundMask ( this );

        if ( ! set_mask ( newMask ) ) {
            BOOST_LOG_TRIVIAL(error) <<  "Unable to add mask to ExtendedCompoundImage with mirrors" ;
            return false;
        }
    }
    
    calculate_offsets();

    return true;
}

bool ExtendedCompoundImage::extend_bbox ( BoundingBox< double > otherbbox, int morePix = 0 ) {

    BoundingBox<double> newBbox ( bbox );
    double nbPix;

    /******************** Mise à jour des dimensions **********************/

    //XMIN
    if ( otherbbox.xmin < bbox.xmin ) {
        nbPix = ( double ) ceil ( ( bbox.xmin - otherbbox.xmin ) / resx );
        BOOST_LOG_TRIVIAL(debug) <<  "Ajout de " << nbPix << " à gauche" ;
        width += (int) nbPix;
        newBbox.xmin -= nbPix * resx;
    }

    // XMAX
    if ( otherbbox.xmax > bbox.xmax ) {
        nbPix = ( double ) ceil ( ( otherbbox.xmax - bbox.xmax ) / resx );
        BOOST_LOG_TRIVIAL(debug) <<  "Ajout de " << nbPix << " à droite" ;
        width += (int) nbPix;
        newBbox.xmax += nbPix * resx;
    }

    //YMIN
    if ( otherbbox.ymin < bbox.ymin ) {
        nbPix = ( double ) ceil ( ( bbox.ymin - otherbbox.ymin ) / resy );
        BOOST_LOG_TRIVIAL(debug) <<  "Ajout de " << nbPix << " en bas" ;
        height += (int) nbPix;
        newBbox.ymin -= nbPix * resy;
    }

    // YMAX
    if ( otherbbox.ymax > bbox.ymax ) {
        nbPix = ( double ) ceil ( ( otherbbox.ymax - bbox.ymax ) / resy );
        BOOST_LOG_TRIVIAL(debug) <<  "Ajout de " << nbPix << " en haut" ;
        height += (int) nbPix;
        newBbox.ymax += nbPix * resy;
    }

    /******************** ajout de pixels supplémentaires *****************/

    if ( morePix > 0 ) {
        BOOST_LOG_TRIVIAL(debug) << "Ajout de " << morePix << " pixels de tous les côtés";
        width += 2*morePix;
        height += 2*morePix;

        newBbox.xmin -= morePix * resx;
        newBbox.ymin -= morePix * resy;
        newBbox.xmax += morePix * resx;
        newBbox.ymax += morePix * resy;
    }

    
    if (! Image::are_dimensions_consistent(resx, resy, width, height, newBbox)) {
        BOOST_LOG_TRIVIAL(error) <<  "Resolutions, new bounding box and new pixels dimensions of the enlarged ExtendedCompoundImage are not consistent" ;
        return false;
    }


    /*********************** mise à jour des attributs ********************/

    bbox = newBbox;

    // Mise à jour du masque associé à l'image composée
    if ( mask ) {
        ExtendedCompoundMask* newMask = new ExtendedCompoundMask ( this );

        if ( ! set_mask ( newMask ) ) {
            BOOST_LOG_TRIVIAL(error) <<  "Unable to add mask to enlarged ExtendedCompoundImage" ;
            return false;
        }
    }
    
    calculate_offsets();

    return true;
}

ExtendedCompoundImage* ExtendedCompoundImage::create (
    std::vector<Image*>& images, int* nodata, uint mirrors ) {

    // On doit forcément avoir une image en entrée, car les dimensions de l'ExtendedCompoundImage sont calculée à partir des sources
    if ( images.size() == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "No source images to define compounded image" ;
        return NULL;
    }

    for ( int i=0; i<images.size()-1; i++ ) {
        if ( ! images[i]->compatible ( images[i+1] ) ) {
            BOOST_LOG_TRIVIAL(error) <<  "Source images are not consistent" ;
            BOOST_LOG_TRIVIAL(error) <<  "Image " << i ;
            images[i]->print();
            BOOST_LOG_TRIVIAL(error) <<  "Image " << i+1 ;
            images[i+1]->print();
            return NULL;
        }
    }

    // Rectangle englobant des images d entree
    double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
    for ( unsigned int j=0; j<images.size(); j++ ) {
        if ( images.at ( j )->get_xmin() <xmin )  xmin=images.at ( j )->get_xmin();
        if ( images.at ( j )->get_ymin() <ymin )  ymin=images.at ( j )->get_ymin();
        if ( images.at ( j )->get_xmax() >xmax )  xmax=images.at ( j )->get_xmax();
        if ( images.at ( j )->get_ymax() >ymax )  ymax=images.at ( j )->get_ymax();
    }

    int w = ( int ) ( ( xmax-xmin ) / ( *images.begin() )->get_resx() +0.5 );
    int h = ( int ) ( ( ymax-ymin ) / ( *images.begin() )->get_resy() +0.5 );
    
    ExtendedCompoundImage* pECI = new ExtendedCompoundImage (
        w, h, images.at ( 0 )->get_channels(), ( *images.begin() )->get_resx(), ( *images.begin() )->get_resy(), BoundingBox<double> ( xmin,ymin,xmax,ymax ),
        images, nodata, mirrors
    );
    pECI->set_crs ( images.at ( 0 )->get_crs() );

    return pECI;
}

ExtendedCompoundImage* ExtendedCompoundImage::create (
    int width, int height, int channels, BoundingBox<double> bbox,
    std::vector<Image*>& images, int* nodata, uint mirrors ) {
    
    double resx = -1, resy = -1;

    if ( images.size() == 0 ) {
        // On peut ne pas avoir d'image en entrée, l'ExtendedCompoundImage sera alors pleine de nodata, et le masque sera vide (que des 0)
        BOOST_LOG_TRIVIAL(info) <<  "No source images to compose the ExtendedCompoundImage (=> only nodata)" ;
        resx = (bbox.xmax - bbox.xmin) / (double) width;
        resy = (bbox.ymax - bbox.ymin) / (double) height;
    } else {

        for ( int i=0; i<images.size()-1; i++ ) {
            if ( ! images[i]->compatible ( images[i+1] ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Source images are not consistent" ;
                BOOST_LOG_TRIVIAL(error) <<  "Image " << i ;
                images[i]->print();
                BOOST_LOG_TRIVIAL(error) <<  "Image " << i+1 ;
                images[i+1]->print();
                return NULL;
            }
        }
        
        resx = images.at(0)->get_resx();
        resy = images.at(0)->get_resy();
        
        if (! Image::are_dimensions_consistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and dimensions for ExtendedCompoundImage are not consistent" ;
            return NULL;
        }
    }

    return new ExtendedCompoundImage ( width,height,channels, resx, resy, bbox,images,nodata,mirrors );
}

/********************************************** ExtendedCompoundMask *************************************************/

int ExtendedCompoundMask::_getline ( uint8_t* buffer, int line ) {

    memset ( buffer,0,width );

    for ( uint i = ECI->get_mirrors_count(); i < ECI->get_images()->size(); i++ ) {
        
        int ol, c0, c1, c2;
        
        ECI->get_offsets(i, &ol, &c0, &c1, &c2);
        
        int lineInSource = line - ol;
        
        /* On ecarte les images qui ne se trouvent pas sur la ligne
         * On evite de comparer des coordonnees terrain (comparaison de flottants)
         * Les coordonnees image sont obtenues en arrondissant au pixel le plus proche
         */
        if ( lineInSource < 0 || lineInSource >=  ECI->get_images()->at ( i )->get_height() ) {
            continue;
        }
        if ( ECI->get_images()->at ( i )->get_xmin() >= get_xmax() || ECI->get_images()->at ( i )->get_xmax() <= get_xmin() ) {
            continue;
        }
 
        if ( ECI->get_mask ( i ) == NULL ) {
            memset ( &buffer[c0], 255, c1 - c0 + 1 );
        } else {
            // Récupération du masque de l'image courante de l'ECI.
            uint8_t* buffer_m = new uint8_t[ECI->get_mask ( i )->get_width()];
            ECI->get_mask ( i )->get_line ( buffer_m,lineInSource );
            // On ajoute au masque actuel (on écrase si la valeur est différente de 0)
            for ( int j = 0; j < c1 - c0 + 1; j++ ) {
                if ( buffer_m[c2+j] ) {
                    memcpy ( &buffer[c0+j],&buffer_m[c2+j],1 );
                }
            }
            delete [] buffer_m;
        }
    }

    return width;
}

/* Implementation de get_line pour les uint8_t */
int ExtendedCompoundMask::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de get_line pour les float */
int ExtendedCompoundMask::get_line ( uint16_t* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    get_line ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
}

/* Implementation de get_line pour les float */
int ExtendedCompoundMask::get_line ( float* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    get_line ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
}
