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
 * \file TiffNodataManager.h
 ** \~french
 * \brief Définition de la classe TiffNodataManager, permettant de modifier la couleur de nodata des images à canal entier
 ** \~english
 * \brief Define classe TiffNodataManager, allowing to modify nodata color for byte sample image
 */

using namespace std;

#ifndef _TIFFNODATAMANAGER_
#define _TIFFNODATAMANAGER_

#include <boost/log/trivial.hpp>
#include <stdint.h>
#include <tiff.h>
#include <tiffio.h>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <queue>

#include "rok4/enums/Format.h"
#include "rok4/image/file/FileImage.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation de la couleur de nodata
 * \details On veut pouvoir modifier la couleur de nodata d'images à canal entier, et ce pour des images qui ne possèdent pas de masque de données. De plus, on veut avoir la possibilité de supprimer une couleur d'une image pour la réserver au nodata. Typiquement, il est des cas où le blanc pur doit être exclusivement utilisé pour caractériser des pixels de nodata. On va alors remplacer le blanc de donnée (neige, écume, hot spot...) par du "gris clair" (254,254,254).
 *
 * On va donc définir 3 couleurs :
 * \li la couleur cible #target_value : les pixels de cette couleur sont ceux potentiellement modifiés
 * \li la nouvelle couleur de nodata #nodata_value (peut être la même que celle cible, pour spécifier qu'on ne veut pas la changer)
 * \li la nouvelle couleur de donnée #data_value (peut être la même que celle cible, pour spécifier qu'on ne veut pas la changer)
 *
 * Et la classe va permettre les actions suivantes :
 * \li de modifier la couleur des pixels de nodata
 * \li de modifier la couleur des pixels de données
 * \li écrire le masque correspondant à l'image en entrée et sortie. Si l'image ne contient pas de nodata (on dit alors qu'elle est pleine), le masque n'est pas écrit.
 *
 * Pour identifier les pixels de nodata, on peut utiliser l'option "touche les bords" (#touch_edges) ou non en plus de la valeur cible.
 *
 * On dit qu'un pixel "touche le bord" dès lors que l'on peut relier le pixel au bord en ne passant que par des pixels dont la couleur est celle cible. Techniquement, on commence par les bord puis on se propage vers l'intérieur de l'image.
 *
 * \~ \image html manageNodata.png \~french
 *
 * Les fonctions utilisent les loggers et doivent donc être initialisés par un des appelant.
 */

template<typename T>
class TiffNodataManager {
private:

    /**
     * \~french \brief Largeur de l'image en cours de traitement
     * \~english \brief Width of the treated image
     */
    uint32_t width;

    /**
     * \~french \brief Hauteur de l'image en cours de traitement
     * \~english \brief Height of the treated image
     */
    uint32_t height;

    /**
     * \~french \brief Nombre de canaux de l'image en cours de traitement
     * \~english \brief Number of samples of the treated image
     */
    uint16_t samplesperpixel;


    /**
     * \~french \brief Nombre de canaux des couleurs du manager
     * \~english \brief Number of samples in manager colors
     */
    uint16_t max_channels;

    /**
     * \~french \brief Couleur concerné par les modifications éventuelles
     * \~english \brief Color impacted by modifications
     */
    T *target_value;

    /**
     * \~french \brief Tolérance de la comparaison avec la valeur cible
     * \~english \brief Target value comparison tolerance
     */
    int tolerance;

    /**
     * \~french \brief Méthode de détection des pixels de nodata
     * \details Part-on des bords pour identifier les pixels de nodata ?
     * \~english \brief Nodata pixels detection method
     * \details Do we spread from image's edges to identify nodata pixels ?
     */
    bool touch_edges;

    /**
     * \~french \brief Nouvelle couleur pour les pixels de donnée
     * \details Elle peut être la même que la couleur cible #target_value. Dans ce cas, on ne touche pas aux pixels de donnée.
     * \~english \brief New color for data
     * \details Could be the same as #target_value.
     */
    T *data_value;
    /**
     * \~french \brief Nouvelle couleur pour les pixels de non-donnée
     * \details Elle peut être la même que la couleur cible #target_value. Dans ce cas, on ne touche pas aux pixels de non-donnée.
     * \~english \brief New color for nodata
     * \details Could be the same as #target_value.
     */
    T *nodata_value;

    /**
     * \~french \brief Doit-on modifier les pixels de données contenant la valeur cible #target_value
     * \details Cela peut être utile pour réserver une couleur aux pixels de nodata.
     * \~english \brief Have we to modify data pixel wich contain the target value #target_value
     */
    bool remove_target_value;
    /**
     * \~french \brief Doit-on modifier la valeur des pixels de nodata ?
     * \details On mettra la couleur #nodata_value.
     * \~english \brief Have we to switch the nodata pixels' color ?
     * \details #nodata_value will be put.
     */
    bool new_nodata_value;

    /**
     * \~french \brief Identifie les pixels de nodata
     * \details Les pixels de nodata potentiels sont ceux qui ont la valeur #target_value. Deux méthodes sont disponibles :
     * \li Tous les pixels qui ont cette valeur sont du nodata
     * \li Seuls les pixels qui ont cette valeur et qui "touchent le bord" sont du nodata
     *
     * Pour cette deuxième méthode, plus lourde, nous allons procéder comme suit. On identifie les pixels du bord dont la couleur est #target_value, et on les ajoute dans une pile (on stocke la position en 1 dimension dans l'image). On remplit en parallèle le masque de donnée, pour mémoriser les pixels identifié comme nodata.
     *
     * Itérativement, tant que la pile n'est pas vide, on en prend la tête et on considère les 4 pixels autour. Si ils sont de la couleur #data_value et qu'ils n'ont pas déjà été traités (on le sait grâce au masque), on les ajoute à la pile. Ainsi de suite jusqu'à vider la pile.
     *
     * Dans les deux acs, on remplit un buffer qui servira de masque afin de, au choix :
     * \li changer la valeur de nodata
     * \li changer la valeur des mixels de données qui ont la valeur #target_value (réserver une couleur aux pixels de nodata)
     * \li écrire le masque dans un fichier
     *
     * \param[in] IM image à analyser
     * \param[out] MSK masque à remplir
     *
     * \return VRAI si l'image contient au moins 1 pixel de nodata, FAUX si elle n'en contient pas
     *
     * \~english \brief Identify nodata pixels
     * \param[in] IM image to analyze
     * \param[out] MSK mask to fill
     * \return TRUE if the image sontains 1 nodata pixel or more, FALSE otherwise
     */
    bool identify_nodata_pixels ( T* IM, uint8_t* MSK );

    /**
     * \~french \brief Détermine si un pixel contient la valeur cible
     * \details Un pixel est considéré comme de la couleur cible s'il appartient à l'intervalle définit par #target_value et #tolerance
     * \param[in] pix pixel à tester
     * \return
     *
     * \~english \brief Determine target value pixel
     * \param[in] pix pixel to test
     */
    inline bool is_target_value ( T* pix );

    /**
     * \~french \brief Change la couleur des pixels de nodata
     * \details Les pixels de nodata ont déjà été identifiés. Reste à en changer la valeur par #nodata_value.
     *
     * \param[in,out] IM image à modifier
     * \param[in] MSK masque à utiliser
     *
     * \~english \brief Switch color of nodata pixels
     * \param[in,out] IM image to modify
     * \param[in] MSK mask to use
     */
    void change_nodata_value ( T* IM, uint8_t* MSK );

    /**
     * \~french \brief Change la couleur des pixels de donnée
     * \details Les pixels de nodata ont déjà été identifiés. Il se peut que des pixels de données soit de la couleur #target_value et qu'on veuille la changer (réserver une couleur aux pixels de nodata). On parcourt l'image et on change la valeur de ces derniers par #data_value.
     *
     * \param[in,out] IM image à modifier
     * \param[in] MSK masque à utiliser
     *
     * \~english \brief Switch color of data pixels
     * \param[in,out] IM image to modify
     * \param[in] MSK mask to use
     */
    void change_data_value ( T* IM, uint8_t* MSK );

public:

    /** \~french
     * \brief Crée un objet TiffNodataManager à partir des différentes couleurs
     * \details Les booléens #remove_target_value et #new_nodata_value sont déduits des couleurs, si elles sont identiques ou non.
     * \param[in] channels nombre de canaux dans les couleurs
     * \param[in] target_value couleur cible
     * \param[in] touch_edges méthode de détection des pixels de nodata
     * \param[in] data_value nouvelle couleur pour les données
     * \param[in] nodata_value nouvelle couleur pour les non-données
     ** \~english
     * \brief Create a TiffNodataManager object from three characteristic colors
     * \details Booleans #remove_target_value and #new_nodata_value are deduced from colors.
     * \param[in] channels colors' number of samples
     * \param[in] target_value color to treat
     * \param[in] touch_edges Nodata pixels detection method
     * \param[in] data_value new color for data
     * \param[in] nodata_value new color for nodata
     */
    TiffNodataManager ( uint16 channels, int* target_value, bool touch_edges,
                        int* data_value, int* nodata_value, int tolerance = 0 );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~TiffNodataManager() {
        delete[] target_value;
        delete[] nodata_value;
        delete[] data_value;
    }

    /** \~french
     * \brief Fonction de traitement du manager, effectuant les modification de l'image
     * \details Elle utilise les booléens #remove_target_value et #new_nodata_value pour déterminer le travail à faire. Si le travail consisite simplement à identifier le nodata et écrire un maque (pas de modification à apporter à l'image), l'image ne sera pas réecrite, même si un chemin différent pour la sortie est fourni.
     * \param[in] input chemin de l'image à modifier
     * \param[in] output chemin de l'image de sortie
     * \param[in] output_mask_path chemin du masque de sortie
     * \return Vrai en cas de réussite, faux sinon
     ** \~english
     * \brief Manager treatment function, doing image's modifications
     * \details Use booleans #remove_target_value and #new_nodata_value to define what to do.
     * \param[in] input Image's path to modify
     * \param[in] output Output image path
     * \param[in] output Output mask path
     * \return True if success, false otherwise
     */
    bool process_nodata ( char* input_image_path, char* output_image_path, char* output_mask_path = 0 );

};




/*****************************************************************************************************/
/******************************************** DEFINITIONS ********************************************/
/*****************************************************************************************************/




template<typename T>
TiffNodataManager<T>::TiffNodataManager ( uint16 channels, int* tv, bool touch_edges, int* dv, int* nv, int t ) :
    max_channels ( channels ), touch_edges ( touch_edges ), tolerance ( t ) {

    target_value = new T[channels];
    data_value = new T[channels];
    nodata_value = new T[channels];

    for ( int i = 0; i < channels; i++ ) {
        target_value[i] = ( T ) tv[i];
        data_value[i] = ( T ) dv[i];
        nodata_value[i] = ( T ) nv[i];
    }

    if ( memcmp ( tv,nv,channels*sizeof ( int ) ) ) {
        new_nodata_value = true;
    } else {
        // La nouvelle valeur de non-donnée est la même que la couleur cible : on ne change pas la couleur de non-donnée
        new_nodata_value = false;
    }

    if ( touch_edges && memcmp ( tv,dv,channels*sizeof ( int ) ) ) {
        // Pour changer la couleur des données contenant la couleur cible, il faut avoir l'option "touche les bords".
        // Sinon, par définition, aucun pixel de la couleur cible est à considérer comme de la donnée.
        remove_target_value = true;
    } else {
        // La nouvelle valeur de donnée est la même que la couleur cible : on ne supprime donc pas la couleur cible des données
        remove_target_value = false;
    }
}

template<typename T>
bool TiffNodataManager<T>::process_nodata ( char* input_image_path, char* output_image_path, char* output_mask_path ) {
    if ( ! new_nodata_value && ! remove_target_value && ! output_mask_path ) {
        BOOST_LOG_TRIVIAL(info) <<  "Have nothing to do !" ;
        return true;
    }

    FileImage* source_image = FileImage::create_to_read(input_image_path);

    if ( source_image == NULL )  {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot create the input image "<< input_image_path ;
        return false;
    }
    
    /* On mémorise certaines informations sur l'image en cours de traitement */
    width = source_image->get_width();
    height = source_image->get_height();

    Photometric::ePhotometric photometric = source_image->get_photometric();
    Compression::eCompression compression = source_image->get_compression();
    SampleFormat::eSampleFormat sample_format = source_image->get_sample_format();
    samplesperpixel = source_image->get_channels();
    
    if ( samplesperpixel > max_channels )  {
        BOOST_LOG_TRIVIAL(error) << "The nodata manager is not adapted (samplesperpixel have to be " << max_channels <<
                       " or less) for the image " << input_image_path << " (" << samplesperpixel << ")";
        return false;
    }

    /*************** Chargement de l'image ***************/

    T *image_buffer = (T*) malloc (width * height * samplesperpixel * sizeof(T));
    if ( image_buffer == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot allocate a buffer of size " << width * height * samplesperpixel * sizeof(T) << " bytes" ;
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "We load input image into memory : " << width * height * samplesperpixel * sizeof(T) / 1024 << " kilobytes";
    for ( int h = 0; h < height; h++ ) {
        if (source_image->get_line(image_buffer + width*samplesperpixel*h, h) == 0) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot read line " << h ;
            return false;
        }
    }

    delete source_image;

    BOOST_LOG_TRIVIAL(debug) << "Premier pixel : " << (int)image_buffer[0] << "," << (int)image_buffer[1] << "," << (int)image_buffer[2];

    /************* Calcul du masque de données ***********/

    uint8_t *mask_buffer = new uint8_t[width * height];

    bool contain_nodata = identify_nodata_pixels ( image_buffer, mask_buffer );

    /*************** Modification des pixels *************/

    if ( remove_target_value ) {
        BOOST_LOG_TRIVIAL(debug) << "The 'target_value' data pixels are replaced by 'data_value' pixels";
        change_data_value ( image_buffer, mask_buffer );
    }

    if ( new_nodata_value ) {
        BOOST_LOG_TRIVIAL(debug) << "Nodata pixels which touch edges are replaced by 'nodata_value' pixels";
        change_nodata_value ( image_buffer, mask_buffer );
    }

    /**************** Ecriture de l'images ****************/

    /* Seulement si on a modifié l'image. Si seule l'écriture du masque nous intéressait, on ne réecrit pas l'image,
     * même si un chemin d'image différent est fourni pour la sortie */
    if ( remove_target_value || new_nodata_value ) {

        FileImage* output_image = FileImage::create_to_write(
            output_image_path, BoundingBox<double>(0,0,0,0), -1, -1, width, height,
            samplesperpixel, sample_format, photometric, compression
        );

        if ( output_image == NULL )  {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot create the output image "<< output_image_path ;
            return false;
        }
        
        BOOST_LOG_TRIVIAL(debug) << "We write the output image " << output_image_path;
        output_image->write_image(image_buffer);

        delete output_image;

    } else {
        if ( memcmp ( input_image_path, output_image_path, sizeof ( output_image_path ) ) )
            BOOST_LOG_TRIVIAL(info) <<  "The image have not be modified, the file '" << output_image_path <<"' is not written" ;
    }

    /**************** Ecriture du masque ? ****************/
    if ( output_mask_path ) {
        if (contain_nodata) {
            FileImage* output_mask = FileImage::create_to_write(
                output_mask_path, BoundingBox<double>(0,0,0,0), -1, -1, width, height,
                1, SampleFormat::UINT8, Photometric::MASK, Compression::DEFLATE
            );

            if ( output_mask == NULL )  {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot create the output mask "<< output_mask_path ;
                return false;
            }

            BOOST_LOG_TRIVIAL(debug) << "We write the output mask " << output_mask_path;
            output_mask->write_image(mask_buffer);

            delete output_mask;
        } else {
            BOOST_LOG_TRIVIAL(info) <<  "The image contains only data, the mask '" << output_mask_path <<"' is not written" ;
        }
    }

    free(image_buffer);
    delete[] mask_buffer;

    return true;
}

template<typename T>
inline bool TiffNodataManager<T>::is_target_value ( T* pix ) {
    int pixint;
    int tvint;
    for ( int i = 0; i < samplesperpixel; i++ ) {
        pixint = ( int ) pix[i];
        tvint = ( int ) target_value[i];
        if ( pixint < tvint - tolerance || pixint > tvint + tolerance ) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool TiffNodataManager<T>::identify_nodata_pixels ( T* IM, uint8_t* MSK ) {

    BOOST_LOG_TRIVIAL(debug) <<  "Identify nodata pixels..." ;
    memset ( MSK, 255, width * height );

    bool containNodata = false;

    if ( touch_edges ) {
        BOOST_LOG_TRIVIAL(debug) <<  "\t...which touch edges" ;
        // On utilise la couleur target_value et on part des bords
        queue<unsigned long long> Q;

        // Initialisation : we identify front pixels which are 'target_value'
        for ( unsigned long long pos = 0; pos < width; pos++ ) { // top
            if ( is_target_value ( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( unsigned long long pos = width* ( height-1 ); pos < width*height; pos++ ) { // bottom
            if ( is_target_value ( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( unsigned long long pos = 0; pos < width*height; pos += width ) { // left
            if ( is_target_value ( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( unsigned long long pos = width -1; pos < width*height; pos+= width ) { // right
            if ( is_target_value ( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }

        if ( Q.empty() ) {
            // No nodata pixel identified, nothing to do
            return false;
        }

        containNodata = true;

        // while there are 'target_value' pixels which can propagate, we do it
        while ( !Q.empty() ) {
            unsigned long long pos = Q.front();
            Q.pop();
            unsigned long long newpos;
            if ( pos % width > 0 ) {
                newpos = pos - 1;
                if ( MSK[newpos] && is_target_value ( IM + newpos*samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos % width < width - 1 ) {
                newpos = pos + 1;
                if ( MSK[newpos] && is_target_value ( IM + newpos*samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width > 0 ) {
                newpos = pos - width;
                if ( MSK[newpos] && is_target_value ( IM + newpos*samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width < height - 1 ) {
                newpos = pos + width;
                if ( MSK[newpos] && is_target_value ( IM + newpos*samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
        }

    } else {
        BOOST_LOG_TRIVIAL(debug) <<  "\t..., all pixels in 'target color'" ;
        // Tous les pixels de la couleur target_value sont à considérer comme du nodata

        for ( unsigned long long i = 0; i < width * height; i++ ) {

            if ( is_target_value ( IM+i*samplesperpixel ) ) {
                containNodata = true;
                MSK[i] = 0;
            }
        }

    }

    return containNodata;
}

template<typename T>
void TiffNodataManager<T>::change_nodata_value ( T* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( ! MSK[i] ) {
            memcpy ( IM+i*samplesperpixel,nodata_value,samplesperpixel*sizeof ( T ) );
        }
    }
}

template<typename T>
void TiffNodataManager<T>::change_data_value ( T* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( MSK[i] && is_target_value ( IM+i*samplesperpixel ) ) {
            memcpy ( IM+i*samplesperpixel,data_value,samplesperpixel*sizeof ( T ) );
        }
    }
}

#endif // _TIFFNODATAMANAGER_
