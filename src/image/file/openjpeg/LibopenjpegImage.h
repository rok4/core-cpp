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
 * \file LibopenjpegImage.h
 ** \~french
 * \brief Définition des classes LibopenjpegImage et LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 * \li LibopenjpegImageFactory : usine de création d'objet LibopenjpegImage
 ** \~english
 * \brief Define classes LibopenjpegImage and LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : manage a JPEG2000 format image, reading, using the library openjpeg
 * \li LibopenjpegImageFactory : factory to create LibopenjpegImage object
 */

#ifndef LIBOPENJPEG_IMAGE_H
#define LIBOPENJPEG_IMAGE_H

#include "utils/BoundingBox.h"
#include "image/file/FileImage.h"
#include "image/Image.h"
#include <openjpeg.h>

#define JP2_RFC3745_MAGIC    "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC            "\x0d\x0a\x87\x0a"
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image JPEG2000, avec la librarie openjpeg
 * \details Une image JPEG2000 est une vraie image dans ce sens où elle est rattachée à un fichier, pour la lecture de données au format JPEG2000. La librairie utilisée est openjpeg (open source et intégrée statiquement dans le projet ROK4).
 */
class LibopenjpegImage : public FileImage {
    
friend class LibopenjpegImageFactory;
    
private:
     
    /**
     * \~french \brief Largeur pixel de la tuile, 0 si la donnée n'est pas tuilée
     * \~english \brief Tile pixel width, 0 if data is not tiled
     */
    int tile_width;
    /**
     * \~french \brief Nombre de ligne dans un strip
     * \~english \brief Number of line in one strip
     */
    int rowsperstrip;
    /**
     * \~french \brief Buffer de lecture, de taille strip_size
     * \~english \brief Read buffer, strip_size long
     */
    uint8_t* strip_buffer;
    /**
     * \~french \brief Indice du strip en mémoire dans strip_buffer
     * \~english \brief Memorized strip indice, in strip_buffer
     */
    int current_strip;

    /**
     * \~french \brief Paramètres OpenJPEG
     * \~english \brief OpenJPEG parameters
     */
    opj_dparameters_t jp2_parameters;

    /**
     * \~french \brief Format du codec OpenJPEG
     * \~english \brief OpenJPEG codec format
     */
    OPJ_CODEC_FORMAT jp2_codec;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

protected:
   
    /** \~french
     * \brief Crée un objet LibopenjpegImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine LibopenjpegImageFactory, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] name chemin du fichier image
     * \param[in] sampleformat format des canaux
     * \param[in] photometric photométrie des données
     * \param[in] compression compression des données
     * \param[in] image pointeur vers l'objet image OpenJPEG
     * \param[in] stream pointeur vers l'objet stream OpenJPEG
     * \param[in] codec pointeur vers l'objet codec OpenJPEG
     * \param[in] rowsperstrip taille de la bufferisation des données, en nombre de lignes
     * \param[in] tw Largeur pixel de la tuile, 0 si la donnée n'est pas tuilée
     ** \~english
     * \brief Create a LibopenjpegImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] name path to image file
     * \param[in] sampleformat samples' format
     * \param[in] photometric data photometric
     * \param[in] compression data compression
     * \param[in] parameters OpenJPEG parameters
     * \param[in] rowsperstrip data buffering size, in line number
     * \param[in] tw Tile pixel width, 0 if data is not tiled
     */
    LibopenjpegImage (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, std::string name,
        SampleFormat::eSampleFormat sampleformat, Photometric::ePhotometric photometric, Compression::eCompression compression,
        opj_dparameters_t parameters, OPJ_CODEC_FORMAT codec_format, int rowsperstrip, int tw
    );

public:

    int getline ( uint8_t* buffer, int line );
    int getline ( uint16_t* buffer, int line );
    int getline ( float* buffer, int line );
    

    /**
     * \~french
     * \brief Ecrit une image JPEG2000, à partir d'une image source
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image JPEG2000, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint8_t* buffer ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image JPEG2000, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint16_t* buffer ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image JPEG2000, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( float* buffer)  {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image JPEG2000, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint8_t* buffer, int line ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image JPEG2000, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint16_t* buffer, int line ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image JPEG2000, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format JPEG2000, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( float* buffer, int line) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write JPEG2000 image" ;
        return -1;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression des buffers de lecture
     * \~english
     * \brief Default destructor
     * \details We remove read buffers
     */
    ~LibopenjpegImage() {
        delete [] strip_buffer;
    }

    /** \~french
     * \brief Sortie des informations sur l'image JPEG2000
     ** \~english
     * \brief JPEG2000 image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "---------- LibopenjpegImage ------------" ;
        FileImage::print();
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

};


/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image JPEG2000, manipulée avec la librairie openjpeg
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe LibopenjpegImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de LibopenjpegImage et de sortir en erreur en cas de problème. Dans le cas d'une image JPEG2000 pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image.
 */
class LibopenjpegImageFactory {
public:
    /** \~french
     * \brief Crée un objet LibopenjpegImage, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le JPEG2000, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes JPEG2000. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier JPEG2000.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet LibopenjpegImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an LibopenjpegImage object, for reading
     * \details Bbox and resolutions are not present in the JPEG2000 file, so we precise them. All other informations are extracted from JPEG2000 header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a LibopenjpegImage object pointer, NULL if error
     */
    LibopenjpegImage* createLibopenjpegImageToRead ( std::string filename, BoundingBox<double> bbox, double resx, double resy );

};


#endif

