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
 * \file ExtendedCompoundImage.h
 ** \~french
 * \brief Définition des classes ExtendedCompoundImage, ExtendedCompoundMask
 * \details
 * \li ExtendedCompoundImage : image composée d'images compatibles, superposables
 * \li ExtendedCompoundMask : masque composé, associé à une image composée
 ** \~english
 * \brief Define classes ExtendedCompoundImage, ExtendedCompoundMask
 * \details
 * \li ExtendedCompoundImage : image compounded with superimpose images
 * \li ExtendedCompoundMask : compounded mask, associated with a compounded image
 */

#pragma once

#include <vector>
#include <cstring>
#include <iostream>
#include <math.h>

#include <boost/log/trivial.hpp>
#include "rok4/utils/Utils.h"
#include "rok4/enums/Format.h"
#include "rok4/image/Image.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un paquet d'images compatibles
 * \details On va manipuler un paquet d'images superposables comme si elles n'en étaient qu'une seule. On parle d'images superposables lorsqu'elles ont :
 * \li le même système spatial
 * \li la même résolution en X
 * \li la même résolution en Y
 * \li la même phase en X
 * \li la même phase en Y
 *
 * Les tests d'égalité acceptent un epsilon qui est le suivant :
 * \li 1% de la résolution la plus petite pour les résolutions
 * \li 1% pour les phases
 *
 * \~ \image html eci.png
 */
class ExtendedCompoundImage : public Image {

private:

    /**
     * \~french \brief Images sources, toutes superposables, utilisée pour assembler l'image composée
     * \details La première image est celle du dessous
     * \~english \brief Source images, consistent , to make the compounded image
     * \details First image is the bottom one
     */
    std::vector<Image*> source_images;
    
    /**
     * \~french \brief Offset sur les lignes
     * \details Numéro de la ligne dans l'ExtendedCompoundImage correspondant à la première ligne de l'image source
     * \~english \brief Row's offset
     */
    std::vector<int> rows_offsets;
    
    /**
     * \~french \brief Offset sur les colonnes
     * \details Numéro de la colonne dans l'ExtendedCompoundImage correspondant à la première colonne de l'image source, ou 0 si c'est négatif
     * \~english \brief Columns's offset
     */
    std::vector<int> c0s;
    
    /**
     * \~french \brief Offset sur les colonnes
     * \details Numéro de la colonne dans l'ExtendedCompoundImage correspondant à la dernière colonne de l'image source, ou width - 1 si c'est supérieur
     * \~english \brief Columns's offset
     */
    std::vector<int> c1s;
    
    /**
     * \~french \brief Offset sur les colonnes
     * \details Numéro de la colonne dans l'image source correspondant à la première colonne de ExtendedCompoundImage, ou 0 si c'est négatif
     * \~english \brief Columns's offset
     */
    std::vector<int> c2s;

    /**
     * \~french \brief Nombre de miroirs dans les images sources
     * \details Certaines images sources peuvent etre des miroirs (MirrorImage). Lors de la composition de l'image, on ne veut pas que les données des vraies images soient écrasées par des données "miroirs". C'est pourquoi on veut connaître le nombre d'images miroirs dans le tableau et on sait qu'elles sont placées au début.
     * \~english \brief Mirror images' number among source images
     */
    uint mirrors_count;

    /**
     * \~french \brief Valeur de non-donnée
     * \details On a une valeur entière par canal. Tous les pixel de l'image composée seront initialisés avec cette valeur.
     * \~english \brief Nodata value
     */
    int* nodata_value;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \details Lorsque l'on veut récupérer une ligne d'une image composée, on va se reporter sur toutes les images source.
     *
     * \image html eci_getline.png
     *
     * On lit en parallèle de chaque image source l'éventuel masque qui lui est associé. Ainsi, si un pixel n'est pas réellement de la donnée, on évite d'écraser les données du dessous. Si il n'y a pas de masque, on considère l'image source comme pleine (ne contient pas de non-donnée).
     *
     * \param[in] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );
    
    /** \~french
     * \brief Calcule les offsets pour chaque image source
     * \details Les offsets à caculer sont :
     * \li #rows_offsets
     * \li #c0s
     * \li #c1s
     * \li #c2s
     */
    void calculate_offsets() {
        rows_offsets.clear();
        c0s.clear();
        c1s.clear();
        c2s.clear();
        
        for ( int i = 0; i < ( int ) source_images.size(); i++ ) {
            
            double y = source_images[i]->line_to_y ( 0 );
            
            rows_offsets.push_back(y_to_line ( y ));
            c0s.push_back(std::max ( 0,x_to_column ( source_images[i]->get_xmin() + 0.5*source_images[i]->get_resx() ) ));
            c1s.push_back(std::min ( width - 1,x_to_column ( source_images[i]->get_xmax() - 0.5*source_images[i]->get_resx() ) ));
            c2s.push_back(std::max ( 0, source_images[i]->x_to_column ( bbox.xmin + 0.5*resx ) ) );
        }
    }

    /** \~french
     * \brief Crée un objet ExtendedCompoundImage à partir de tous ses éléments constitutifs
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] images images sources
     * \param[in] nd valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     ** \~english
     * \brief Create an ExtendedCompoundImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     * \param[in] images source images
     * \param[in] nd nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     */
    ExtendedCompoundImage ( int width, int height, int channels, double resx, double resy, BoundingBox<double> bbox,
                            std::vector<Image*>& images, int* nd, uint mirrors ) :
        Image ( width, height, channels, resx, resy, bbox ),
        source_images ( images ),
        mirrors_count ( mirrors ) {

        nodata_value = new int[channels];
        memcpy ( nodata_value,nd,channels*sizeof ( int ) );
        
        calculate_offsets();
    }

public:
    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image composée et crée un objet ExtendedCompoundImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres. On vérifie la superposabilité des images sources.
     * \param[in] images images sources
     * \param[in] nodata valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     * \return un pointeur d'objet ExtendedCompoundImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate compounded image components and create an ExtendedCompoundImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters. We check if source images are superimpose.
     * \param[in] images source images
     * \param[in] nodata nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     * \return a ExtendedCompoundImage object pointer, NULL if error
     */
    static ExtendedCompoundImage* create ( std::vector<Image*>& images, int* nodata, uint mirrors );

    /** \~french
     * \brief Vérifie la superposabilité des images sources et crée un objet ExtendedCompoundImage
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] images images sources
     * \param[in] nodata valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     * \return un pointeur d'objet ExtendedCompoundImage, NULL en cas d'erreur
     ** \~english
     * \brief Check if source images are superimpose and create an ExtendedCompoundImage object
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] images source images
     * \param[in] nodata nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     * \return a ExtendedCompoundImage object pointer, NULL if error
     */
    static ExtendedCompoundImage* create ( int width, int height, int channels,
            BoundingBox<double> bbox,
            std::vector<Image*>& images, int* nodata, uint mirrors );

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
     * \brief Retourne les différents offsets pour l'image demandée, pour les lignes et les colonnes
     * \return offsets
     * \~english
     * \brief Return asked image's offsets, for columns and lines
     * \return offsets
     */
    void get_offsets(int i, int* ol, int* c0, int* c1, int* c2) {
        *ol = rows_offsets[i];
        *c0 = c0s[i];
        *c1 = c1s[i];
        *c2 = c2s[i];
    }

    /**
     * \~french
     * \brief Précise si au moins une image source possède un masque de donnée
     * \return présence de masque
     * \~english
     * \brief Precise if one or more source images own a mask
     * \return mask presence
     */
    bool use_masks() {
        for ( uint i=0; i < source_images.size(); i++ ) {
            if ( get_mask ( i ) ) return true;
        }
        return false;
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
     * \brief Retourne l'image source d'indice i
     * \param[in] i indice de l'image source voulue
     * \return image
     * \~english
     * \brief Return the source images with indice i
     * \param[in] i wanted source image indice
     * \return image
     */
    Image* get_source_image ( int i ) {
        return source_images.at ( i );
    }

    /**
     * \~french \brief Ajoute des miroirs à chacune des images sources
     * \details On va vouloir réechantillonner (ou reprojeter) ce paquet d'images, donc utiliser une interpolation. Une interpolation se fait sur un nombre plus ou moins grand de pixels sources, selon le type. On veut que l'interpolation soit possible même sur les pixels du bord, et ce sans effet de bord.
     *
     * On ajoute donc des pixels virtuels, qui ne sont que le reflet des pixels de l'image. On crée ainsi 4 images miroirs (objets de la classe MirrorImage) par image du paquet (une à chaque bord). On sait distinguer les vraies images de celles virtuelles.
     *
     * On aura préalablement optimisé la taille des miroirs, pour qu'elle soit juste suffisante pour l'interpolation.
     *
     * \param[in] mirrorSize taille en pixel des miroirs, dépendant du mode d'interpolation et du ratio des résolutions
     * \return VRAI dans le cas d'un succès, FAUX sinon.
     * \~english \brief Add mirrors to source images
     * \~ \image html miroirs.png \~french
     */
    bool add_mirrors ( int mirrorSize );

    /**
     * \~french \brief Modifie les dimensions de l'image
     * \details On veut potentiellement augmenter l'étendue de l'image. Cela implique donc de changer les dimensions pixel #width et #height (pas de modification des résolutions). On vérifie bien que la nouvelle bounding box garde la même phase que l'ancienne. Étant donné qu'on n'ajoute pas de nouvelle image source, on augmente donc la quantité de nodata dans l'image.
     *
     * L'extension se déroule en 3 étapes :
     * \li augmentation afin d'inclure la bbox fournie : on ajoute des nombres entiers de pixel à gauche,à droite, en haut et en bas
     * \li on ajoute optionnellement un nombre fixe de pixels des 4 côtés
     * \li on met à jour les attributs de l'ExtendedCompoundImage : les dimensions pixel, la bounding box et éventuellement le masque associé
     *
     * \param[in] otherbbox rectangle englobant à inclure dans celui de l'image
     * \param[in] morePix taille en pixel à ajouter de chaque côté, en plus de vouloir inclure "otherbbox"
     * \return VRAI dans le cas d'un succès, FAUX sinon.
     * \~english \brief Modify image's dimensions
     */
    bool extend_bbox ( BoundingBox<double> otherbbox, int morePix );

    /**
     * \~french
     * \brief Retourne le nombre de miroirs parmi les images sources
     * \return nombre d'images miroirs
     * \~english
     * \brief Return the mirror images' number among source images
     * \return mirror images' number
     */
    uint get_mirrors_count() {
        return mirrors_count;
    }

    /**
     * \~french
     * \brief Retourne la valeur de non-donnée
     * \return Valeur de non-donnée
     * \~english
     * \brief Return the nodata value
     * \return nodata value
     */
    int* get_nodata_value() {
        return nodata_value;
    }

    int get_line ( uint8_t* buffer, int line );
    int get_line ( float* buffer, int line );
    int get_line ( uint16_t* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression de toutes les images composant l'ExtendedCompoundImage
     * \~english
     * \brief Default destructor
     */
    virtual ~ExtendedCompoundImage() {
        delete[] nodata_value;
        if ( ! is_mask ) {
            for ( uint i=0; i < source_images.size(); i++ ) {
                delete source_images[i];
            }
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image composée
     ** \~english
     * \brief Compounded image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ ExtendedCompoundImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Number of images = " << source_images.size() << ", whose " << mirrors_count << " mirrors" ;
        
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- offsets : line, c0, c1, c2";
        for ( int i = 0; i < ( int ) source_images.size(); i++ ) {
            BOOST_LOG_TRIVIAL(info) <<  "\t\t- image " << i << " : " << rows_offsets[i] << ", " << c0s[i] << ", " << c1s[i] << ", " << c2s[i];
            //source_images[i]->print();
        }
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }
};



/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un masque composé, s'appuyant sur une image composée
 */
class ExtendedCompoundMask : public Image {

private:
    /**
     * \~french \brief Image composée, à laquelle le masque composé est associé
     * \~english \brief Compounded images, with which compounded mask is associated
     */
    ExtendedCompoundImage* ECI;

    /** \~french
     * \brief Retourne une ligne entière
     * \details Lors ce que l'on veut récupérer une ligne d'un masque composé, on va se reporter sur tous les masques des images source de l'image composée associée. Si une des images sources n'a pas de masque, on considère que celle-ci est pleine (ne contient pas de non-donnée).
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    int _getline ( uint8_t* buffer, int line );

public:
    /** \~french
     * \brief Crée un ExtendedCompoundMask
     * \details Les caractéristiques du masque sont extraites de l'image composée.
     * \param[in] ECI Image composée
     ** \~english
     * \brief Create a ExtendedCompoundMask
     * \details Mask's components are extracted from the compounded image.
     * \param[in] ECI Compounded image
     */
    ExtendedCompoundMask ( ExtendedCompoundImage* ECI ) :
        Image ( ECI->get_width(), ECI->get_height(), 1, ECI->get_resx(), ECI->get_resy(),ECI->get_bbox() ),
        ECI ( ECI ) {}

    int get_line ( uint8_t* buffer, int line );
    int get_line ( float* buffer, int line );
    int get_line ( uint16_t* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~ExtendedCompoundMask() {}

    /** \~french
     * \brief Sortie des informations sur le masque composé
     ** \~english
     * \brief Compounded mask description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ ExtendedCompoundMask -------" ;
        Image::print();
    }

};


