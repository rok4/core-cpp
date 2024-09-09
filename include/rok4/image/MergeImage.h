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
 * \file MergeImage.h
 ** \~french
 * \brief Définition des classes MergeImage et MergeMask et du namespace Merge
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 * \li Merge : énumère et manipule les différentes méthodes de fusion
 ** \~english
 * \brief Define classes MergeImage and MergeMask and the namespace Merge
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeMask : merged mask, associated with a merged image
 * \li Merge : enumerate and managed different merge methods
 */

#pragma once

#include "rok4/image/Image.h"
#include <string.h>
#include <vector>
#include "rok4/enums/Format.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la méthode de fusion
 * \~english \brief Manage informations in connection with merge method
 */
namespace Merge {
/**
 * \~french \brief Énumération des méthodes de fusion disponibles
 * \~english \brief Available merge methods enumeration
 */
enum eMergeType {
    UNKNOWN = 0,
    NORMAL = 1,
    MULTIPLY = 2,
    ALPHATOP = 3,
    TOP = 4
};

/**
 * \~french \brief Nombre de méthodes disponibles
 * \~english \brief Number of available merge methods
 */
const int mergetype_size = 6;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une méthode de fusion de l'énumération
 * \param[in] strMergeMethod chaîne de caractère à convertir
 * \return la méthode de fusion correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a merge methods enumeration member
 * \param[in] strMergeMethod string to convert
 * \return the binding merge method, UNKNOWN (0) if string is not recognized
 */
eMergeType from_string ( std::string strMergeMethod );

/**
 * \~french \brief Conversion d'une méthode de fusion vers une chaîne de caractères
 * \param[in] merge_method méthode de fusion à convertir
 * \return la chaîne de caractère nommant la méthode de fusion
 * \~english \brief Convert a merge method to a string
 * \param[in] merge_method merge method to convert
 * \return string namming the merge method
 */
std::string to_string ( eMergeType merge_method );
}

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Fusion d'images de mêmes dimensions
 * \details On va manipuler un paquet d'images semblables comme si elles n'en étaient qu'une seule. On parle d'images semblables lorsqu'elles ont :
 * \li les mêmes dimensions
 * \li le format de canaux
 *
 * On va disposer de plusieurs manières de fusionner les images :
 * \li MULTIPLY
 * \li ALPHATOP
 * \li TOP
 * \li NORMAL
 */
class MergeImage : public Image {

private:

    /**
     * \~french \brief Images sources, toutes semblables, utilisée pour assembler l'image fusionnée
     * \details L'image en première position est celle du dessous
     * \~english \brief Source images, similar, to make the merged image
     * \details First image is the bottom one
     */
    std::vector<Image*> source_images;

    /**
     * \~french \brief Méthode d'assemblage des images
     * \~english \brief Way to merge images
     */
    Merge::eMergeType composition;

    /**
     * \~french \brief Valeur de fond
     * \details On a une valeur entière par canal. Tous les pixels de l'image fusionnée seront initialisés avec cette valeur.
     * \~english \brief Background value
     */
    int* background_value;

    /**
     * \~french \brief Valeur de transparence
     * \details On a 3 valeurs entières. Tous les pixels de cette valeur seront considérés comme transparent (en mode TRANSPARENCY)
     * \~english \brief Transparent value
     */
    int* transparent_value;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \param[in] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template <typename T>
    int _getline ( T* buffer, int line );

    /** \~french
     * \brief Crée un objet MergeImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par la méthode statique #create, qui fera différents tests et calculs.
     * \param[in] images images sources
     * \param[in] channel nombre de canaux par pixel en sortie
     * \param[in] bg valeur de pixel à utiliser comme fond, un entier par canal en sortie
     * \param[in] transparent valeur de pixel à considérer comme transparent (peut être NULL), 3 valeurs entières
     * \param[in] composition méthode de fusion à utiliser
     ** \~english
     * \brief Create an MergeImage object, from all attributes
     * \param[in] images source images
     * \param[in] channel number of samples per output pixel
     * \param[in] bg pixel's value to use as background, one integer per output sample
     * \param[in] transparent pixel's value to consider as transparent, 3 integers
     * \param[in] composition merge method to use
     */
    MergeImage ( std::vector< Image* >& images, int channels,
                 int* bg, int* transparent, Merge::eMergeType composition = Merge::NORMAL ) :
        Image ( images.at ( 0 )->get_width(),images.at ( 0 )->get_height(), channels, images.at ( 0 )->get_resx(),images.at ( 0 )->get_resy(), images.at ( 0 )->get_bbox() ),
        source_images ( images ), composition ( composition ), background_value ( bg ), transparent_value ( transparent ) {

        if ( transparent_value != NULL ) {
            transparent_value = new int[3];
            memcpy ( transparent_value, transparent, 3*sizeof ( int ) );
        }

        background_value = new int[channels];
        memcpy ( background_value, bg, channels*sizeof ( int ) );
    }


public:

    virtual int get_line ( uint8_t* buffer, int line );
    virtual int get_line ( uint16_t* buffer, int line );
    virtual int get_line ( float* buffer, int line );

    /**
     * \~french
     * \brief Retourne le tableau des images sources
     * \return images sources
     * \~english
     * \brief Return the array of source images
     * \return source images
     */
    std::vector<Image*>* get_images() {
        return &source_images;
    }

    /**
     * \~french
     * \brief Retourne le masque de l'image source d'indice i
     * \param[in] i indice de l'image source dont on veut le masque
     * \return masque
     * \~english
     * \brief Return the mask of source images with indice i
     * \param[in] i source image indice, whose mask is wanted
     * \return mask
     */
    Image* get_mask ( int i ) {
        return source_images.at ( i )->get_mask();
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression de toutes les images composant la MergeImage
     * \~english
     * \brief Default destructor
     */
    virtual ~MergeImage() {
        if ( ! is_mask ) {
            for ( int i = 0; i < source_images.size(); i++ ) {
                delete source_images[i];
            }
        }
        delete [] background_value;
        if ( transparent_value != NULL ) delete [] transparent_value;
    }

    /** \~french
     * \brief Sortie des informations sur l'image fusionnée
     ** \~english
     * \brief Merged image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ MergeImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Number of images = " << source_images.size() ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Merge method : " << to_string ( composition ) << "\n" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Background value : " << background_value << "\n" ;
    }

    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image fusionnée et crée un objet MergeImage
     * \details Toutes les images sources doivent avoir les même dimensions pixel.
     * \param[in] images images sources
     * \param[in] channel nombre de canaux par pixel en sortie
     * \param[in] background_value valeur de pixel à utiliser comme fond, un entier par canal en sortie
     * \param[in] transparent_value valeur de pixel à considérer comme transparent (peut être NULL), 3 valeurs entières
     * \param[in] composition méthode de fusion à utiliser
     ** \~english
     * \brief Check and calculate compounded image components and create an MergeImage object
     * \details All source images have to own same dimesions.
     * \param[in] images source images
     * \param[in] channel number of samples per output pixel
     * \param[in] background_value pixel's value to use as background, one integer per output sample
     * \param[in] transparent_value pixel's value to consider as transparent, 3 integers
     * \param[in] composition merge method to use
     */
    static MergeImage* create ( std::vector< Image* >& images, int channels,
                                   int* background_value, int* transparent_value, Merge::eMergeType composition = Merge::NORMAL );
};



/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un masque fusionné, s'appuyant sur une image fusionné
 */
class MergeMask : public Image {

private:
    /**
     * \~french \brief Image fusionnée, à laquelle le masque fusionné est associé
     * \~english \brief Merged images, with which merged mask is associated
     */
    MergeImage* MI;

public:
    /** \~french
     * \brief Crée un MergeMask
     * \details Les caractéristiques du masque sont extraites de l'image fusionnée.
     * \param[in] MI Image composée
     ** \~english
     * \brief Create a MergeMask
     * \details Mask's components are extracted from the merged image.
     * \param[in] MI Compounded image
     */
    MergeMask ( MergeImage*& MI ) :
        Image ( MI->get_width(), MI->get_height(), 1,MI->get_resx(), MI->get_resy(),MI->get_bbox() ),
        MI ( MI ) {}

    int get_line ( uint8_t* buffer, int line );
    int get_line ( uint16_t* buffer, int line );
    int get_line ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~MergeMask() {}

    /** \~french
     * \brief Sortie des informations sur le masque fusionné
     ** \~english
     * \brief Merged mask description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ MergeMask -------" ;
        Image::print();
    }

};



