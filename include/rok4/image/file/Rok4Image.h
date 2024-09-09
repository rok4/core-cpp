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
 * \brief Définition des classes Rok4Image
 * \details
 * \li Rok4Image : gestion d'une image aux spécifications ROK4 Server (TIFF tuilé), en écriture et lecture
 ** \~english
 * \brief Define classes Rok4Image
 * \details
 * \li Rok4Image : manage a ROK4 Server specifications image (tiled TIFF), reading and writting
 */

#pragma once

#include <tiffio.h>
#include <string.h>
#include <zlib.h>
#include <jpeglib.h>

#include "rok4/image/Image.h"
#include "rok4/enums/Format.h"
#include "rok4/storage/Context.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image ROK4 Server
 * \details Une image aux specifications ROK4 est une image qui pourra être utilisée par le serveur WMS / WMTS ROK4. Une telle image est au format TIFF, est tuilée, et supporte différentes compressions des données :
 * \li Aucune
 * \li JPEG
 * \li LZW
 * \li Deflate
 * \li PackBit
 * \li PNG : format propre au projet ROK4, cela consiste en des fichiers PNG indépendants (avec en-tête) en lieu et place des tuile de l'image. Ce fonctionnement permet de retourner des tuiles en PNG sans traitement par le serveur, comme pour le format JPEG (format officiel du TIFF).
 *
 * Particularité d'une image ROK4 :
 * \li l'en-tête TIFF fait toujours 2048 (ROK4_IMAGE_HEADER) octets. CEla permet au serveur de ne pas lire cette en-tête, et de passer directement aux données.
 *
 * Toutes les spécifications sont disponible à [cette adresse](https://github.com/rok4/rok4).
 */
class Rok4Image : public Image {

private:

    /**
     * \~french \brief Nom de l'image (sans éventuel contenant objet)
     * \~english \brief image's name (without object tray, if object storage)
     */
    std::string name;

    /**
     * \~french \brief Contexte de stockage de l'image ROK4
     * \~english \brief Image's storage context
     */    
    Context* context;

    /**
     * \~french \brief Nombre de tuile dans le sens de la largeur
     * \~english \brief Number of tiles, widthwise
     */
    int tiles_widthwise;
    /**
     * \~french \brief Nombre de tuile dans le sens de la hauteur
     * \~english \brief Number of tiles, heightwise
     */
    int tiles_heightwise;
    /**
     * \~french \brief Nombre de tuile dans l'image
     * \~english \brief Number of tiles
     */
    int tiles_count;

    
    /******* Pour l'écriture *******/
    
    /**
     * \~french \brief Position du pointeur dans le fichier en cours d'écriture
     * \~english \brief Pointer position in opened written file
     */
    uint32_t position;

    /**
     * \~french \brief Adresse du début de chaque tuile dans le fichier
     * \~english \brief Tile's start address, in the file
     */
    uint32_t *tiles_offsets;
    /**
     * \~french \brief Taille de chaque tuile dans le fichier
     * \~english \brief Tile's size, in the file
     */
    uint32_t *tiles_sizes;


    /**
     * \~french \brief Écrit l'en-tête TIFF de l'image ROK4
     * \details L'en-tête est de taille fixe (ROK4_IMAGE_HEADER_SIZE) et contient toutes les métadonnées sur l'image. Elle ne sera pas lue par le serveur ROK4 (c'est pourquoi sa taille doit être fixe), mais permet de lire l'image avec un logiciel autre (avoir une image TIFF respectant les spécifications).
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write the ROK4 image's TIFF header
     * \return TRUE if success, FALSE otherwise
     */
    bool write_header();
    /**
     * \~french \brief Finalise l'écriture de l'image ROK4
     * \details Cela comprend l'écriture des index et tailles des tuiles
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief End the ROK4 image's writting
     * \return TRUE if success, FALSE otherwise
     */
    bool write_final();

    /**
     * \~french \brief Prépare les buffers pour les éventuelles compressions
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Prepare buffers for compressions
     * \return TRUE if success, FALSE otherwise
     */
    bool prepare_buffers();
    /**
     * \~french \brief Nettoie les buffers de fonctionnement
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Clean temporary buffers
     * \return TRUE if success, FALSE otherwise
     */
    bool clear_buffers();


    /**************************** VECTEUR (Ecriture uniquement) ****************************/

    /**
     * \~french \brief Précise si la donnée est de type vecteur
     * \~english \brief Precis if vector data
     */    
    bool is_vector;

    /**************************** RASTER (Lecture et écriture) ****************************/
    /**
     * \~french \brief Photométrie des données (rgb, gray...)
     * \~english \brief Data photometric (rgb, gray...)
     */
    Photometric::ePhotometric photometric;
    /**
     * \~french \brief type de l'éventuel canal supplémentaire
     * \details En écriture ou dans les traitements, on considère que les canaux ne sont pas prémultipliés par la valeur d'alpha.
     * En lecture, on accepte des images pour lesquelles l'alpha est associé. On doit donc mémoriser cette information et convertir à la volée lors de la lecture des données.
     * \~english \brief extra sample type (if exists)
     */
    ExtraSample::eExtraSample extra_sample;
    /**
     * \~french \brief Compression des données (jpeg, packbits...)
     * \~english \brief Data compression (jpeg, packbits...)
     */
    Compression::eCompression compression;
    /**
     * \~french \brief Format des canaux
     * \~english \brief Sample format
     */
    SampleFormat::eSampleFormat sample_format;
    
    /**
     * \~french \brief Taille d'un pixel en octet
     * \~english \brief Byte pixel's size
     */
    int pixel_size;
    /**
     * \~french \brief Largeur d'une tuile de l'image en pixel
     * \~english \brief Image's tile width, in pixel
     */
    int tile_width;
    /**
     * \~french \brief Hauteur d'une tuile de l'image en pixel
     * \~english \brief Image's tile height, in pixel
     */
    int tile_height;


    /******* Pour la lecture *******/

    /**
     * \~french \brief Taille brut en octet d'une ligne d'une tuile
     * \~english \brief Raw byte size of a tile's line
     */
    int raw_tile_line_size;

    /**
     * \~french \brief Taille brut en octet d'une tuile
     * \~english \brief Raw byte size of a tile
     */
    int raw_tile_size;

    /**
     * \~french \brief Buffer contenant les tuiles mémorisées
     * \details On mémorise les tuiles décompressées
     * \~english \brief Buffer containing memorized tiles
     * \details We memorize uncompressed tiles
     */
    uint8_t *memorized_tiles;

    /**
     * \~french \brief Entier précisant la ligne de tuile déjà chargée et décompressée dans memorized_tiles
     * \details -1 si aucune ligne de tuile n'est mémorisée
     * \~english \brief Tiles line index, loaded uncompressed in memorized_tiles
     * \details -1 if no tile is memorized
     */
    int memorized_tiles_line;
    
    /**
     * \~french \brief Mémorise la ligne de tuiles demandée au format brut (sans compression)
     * \details Si la ligne de tuiles est déjà celle mémorisée dans memorized_tiles (et on le sait grâce à memorizedIndex), on retourne directement OK.
     * \return pointeur vers le tableau contenant la tuile voulue
     * \~english \brief Buffer precising for each memorized tile's indice
     * \return pointer to array containing the wanted tile
     */
    boolean memorize_raw_tiles ( int tilesLine );

    /**
     * \~french \brief Charge l'index des tuiles de l'image ROK4 à lire
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Load index of ROK4 image to read
     * \return TRUE if success, FALSE otherwise
     */
    bool load_index();

    /**
     * \~french \brief Compresse les données brutes en RAW
     * \details Consiste en une simple copie.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into RAW compression
     * \details A simple copy
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_raw_tile ( uint8_t *buffer, uint8_t *data );

     /**
     * \~french \brief Compresse les données brutes en JPEG
     * \details Utilise la libjpeg.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into JPEG compression
     * \details Use libjpeg
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_jpeg_tile ( uint8_t *buffer, uint8_t *data );
    
    /**
     * \~french \brief Compresse les données brutes en LZW
     * \details Utilise la liblzw.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into LZW compression
     * \details Use liblzw.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_lzw_tile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en PACKBITS
     * \details Utilise la libpkb.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into PACKBITS compression
     * \details Use libpkb.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_pkb_tile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en PNG
     * \details Utilise la zlib. Les données retournées contiennent l'en-tête PNG.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into PNG compression
     * \details Use zlib. Returned data contains PNG header.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_png_tile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en DEFLATE
     * \details Utilise la zlib.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into DEFLATE compression
     * \details Use zlib.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t compute_zip_tile ( uint8_t *buffer, uint8_t *data );
    
    template<typename T>
    int _getline ( T* buffer, int line );

    /******* Pour l'écriture *******/

    /**
     * \~french \brief Taille du buffer #Buffer temporaire contenant la tuile à écrire (dans write_tile), compressée
     * \~english \brief Temporary buffer #Buffer size, containing the compressed tile to write
     */
    size_t buffer_size;
    
    /**
     * \~french \brief Buffer temporaire contenant la tuile à écrire (dans write_tile), compressée
     * \~english \brief Buffer size, containing the compressed tile to write
     */
    uint8_t* buffer;

    /**
     * \~french \brief Buffer utilisé par la zlib
     * \details Pour les compressions PNG et DEFLATE uniquement
     * \~english \brief Buffer used by zlib
     */
    uint8_t* zip_buffer;
    /**
     * \~french \brief Flux utilisé par la zlib
     * \details Pour les compressions PNG et DEFLATE uniquement
     * \~english \brief Stream used by zlib
     */
    z_stream zstream;
    
    /**
     * \~french \brief Structure d'informations, utilisée par la libjpeg
     * \details Pour la compression JPEG uniquement
     * \~english \brief Informations structure used by libjpeg
     */
    struct jpeg_compress_struct cinfo;
    /**
     * \~french \brief Structure d'erreur utilisée par la libjpeg
     * \details Pour la compression JPEG uniquement
     * \~english \brief Error structure used by libjpeg
     */
    struct jpeg_error_mgr jerr;


    /**
     * \~french \brief Écrit une tuile de l'image ROK4 raster
     * \details L'écriture tiendra compte de la compression voulue #compression. Les tuiles doivent être écrites dans l'ordre (de gauche à droite, de haut en bas).
     * \param[in] tileInd indice de la tuile à écrire
     * \param[in] data données brutes (sans compression) à écrire
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write a raster ROK4 image's tile
     * \param[in] tileInd tile indice
     * \param[in] data raw data (PBF) to write
     * \return TRUE if success, FALSE otherwise
     */
    bool write_tile ( int tileInd, uint8_t *data );

    /**
     * \~french \brief Écrit une tuile de la dalle ROK4 vecteur
     * \param[in] tileInd indice de la tuile à écrire
     * \param[in] data données brutes (sans compression) à écrire
     * \param[in] pbfpath chemin vers la tuile PBF à écrire dans la dalle
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write a vector ROK4 slab's tile
     * \param[in] tileInd tile indice
     * \param[in] pbfpath path to PBF tile to write in the slab
     * \return TRUE if success, FALSE otherwise
     */
    bool write_tile( int tileInd, char* pbfpath ) ;

    /** \~french
     * \brief Crée un objet Rok4Image raster à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par les méthodes statiques de création, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] name chemin du fichier image
     * \param[in] sample_format format des canaux
     * \param[in] photometric photométrie des données
     * \param[in] compression compression des données
     * \param[in] extra_sample type du canal supplémentaire, si présent.
     * \param[in] tile_width largeur en pixel de la tuile
     * \param[in] tile_height hauteur en pixel de la tuile
     * \param[in] context contexte de stockage
     ** \~english
     * \brief Create a raster Rok4Image object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] name path to image file
     * \param[in] sample_format samples' format
     * \param[in] photometric data photometric
     * \param[in] compression data compression
     * \param[in] extra_sample extra sample type
     * \param[in] tile_width tile's pixel width
     * \param[in] tile_height tile's pixel height
     * \param[in] context storage's context
     */
    Rok4Image (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, std::string name,
        SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric, Compression::eCompression compression, ExtraSample::eExtraSample es,
        int tile_width, int tile_height,
        Context* context
    );
    /** \~french
     * \brief Crée un objet Rok4Image vecteur à partir de tous ses éléments constitutifs
     ** \~english
     * \brief Create a vector Rok4Image object, from all attributes
     */
    Rok4Image ( std::string name, int tilePerWidth, int tilePerHeight, Context* context );

public:
    
    /**
     * \~french
     * \brief Retourne le type du canal supplémentaire
     * \return extra_sample
     * \~english
     * \brief Return extra sample type
     * \return extra_sample
     */
    inline ExtraSample::eExtraSample get_extra_sample() {
        return extra_sample;
    }
    
    /**
     * \~french
     * \brief Modifie le type du canal supplémentaire
     * \~english
     * \brief Modify extra sample type
     */
    inline void set_extra_sample(ExtraSample::eExtraSample es) {
        extra_sample = es;
    }
    /**
     * \~french
     * \brief Retourne la compression des données
     * \return compression
     * \~english
     * \brief Return data compression
     * \return compression
     */
    inline Compression::eCompression get_compression() {
        return compression;
    }

    /**
     * \~french
     * \brief Retourne le format des canaux (entier, flottant)
     * \return format des canaux
     * \~english
     * \brief Return sample format (integer, float)
     * \return sample format
     */
    inline SampleFormat::eSampleFormat get_sample_format() {
        return sample_format;
    }
    
    /**
     * \~french
     * \brief Retourne la photométrie des données image (rgb, gray...)
     * \return photométrie
     * \~english
     * \brief Return data photometric (rgb, gray...)
     * \return photometric
     */
    inline Photometric::ePhotometric get_photometric() {
        return photometric;
    }

    /**
     * \~french
     * \brief Retourne la taille d'une tuile brute (décompressée)
     * \~english
     * \brief Return raw tile size (uncompressed)
     */
    int get_raw_tile_size() {
        return raw_tile_size;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression des buffer de lecture et de l'interface TIFF
     * \~english
     * \brief Default destructor
     * \details We remove read buffer and TIFF interface
     */
    ~Rok4Image() {
        if (! is_vector) {
            delete[] memorized_tiles;
        }
        delete[] tiles_offsets;
        delete[] tiles_sizes;
    }


    /** \~french
     * \brief Sortie des informations sur l'image ROK4
     ** \~english
     * \brief ROK4 image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "---------- Rok4Image ------------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Compression : " << Compression::to_string ( compression ) ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Photometric : " << Photometric::to_string ( photometric ) ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Sample format : " << SampleFormat::to_string ( sample_format ) ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- tile width = " << tile_width << ", tile height = " << tile_height ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Image name : " << name ;
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

    /**************************** Pour la lecture ****************************/
    
    int get_line ( uint8_t* buffer, int line );
    int get_line ( uint16_t* buffer, int line );
    int get_line ( float* buffer, int line );

    /**************************** Pour l'écriture ****************************/

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image.
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_image ( Image* pIn );

    /**
     * \~french
     * \brief Ecrit une dalle ROK4 vecteur, à partir des tuiles PBF
     * \details Toutes les tuiles nécessaires sont contenues dans le dossier #rootDirectory, avec le chemin rootDirectory/column/row.pbf
     * \param[in] ulTileCol Indice de colonne de la tuile supérieure gauche
     * \param[in] ulTileRow Indice de ligne de la tuile supérieure gauche
     * \param[in] rootDirectory Dossier contenant les tuiles PBF
     * \return 0 en cas de succes, -1 sinon
     */
    int writePbfTiles ( int ulTileCol, int ulTileRow, char* rootDirectory );

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_image ( uint8_t* buffer ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image from a buffer" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'un buffer d'entiers 16 bits
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_image ( uint16_t* buffer ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image from a buffer" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_image ( float* buffer ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image from a buffer" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_line ( uint8_t* buffer, int line ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image line by line" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer d'entiers 16 bits
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_line ( uint16_t* buffer, int line) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image line by line" ;
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int write_line ( float* buffer, int line) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot write ROK4 image line by line" ;
        return -1;
    }


    /** \~french
     * \brief Crée un objet Rok4Image, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le TIFF, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes TIFF. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement.
     * Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] name nom de l'image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \param[in] contexte de stockage (fichier, objet ceph ou objet swift)
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create an Rok4Image object, for reading
     * \details Bbox and resolutions are not present in the TIFF file, so we precise them. All other informations are extracted from TIFF header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] name image's name
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \param[in] context storage context (file, ceph object or swift object)
     * \return a Rok4Image object pointer, NULL if error
     */
    static Rok4Image* create_to_read ( std::string name, BoundingBox<double> bbox, double resx, double resy, Context* context );

    /** \~french
     * \brief Crée un objet Rok4Image, pour l'écriture
     * \details Toutes les méta-informations sur l'image doivent être précisées pour écrire l'en-tête TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement.
     * Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     * \param[in] name nom de l'image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] sample_format format des canaux
     * \param[in] photometric photométie des données
     * \param[in] compression compression des données
     * \param[in] tile_width largeur en pixel de la tuile
     * \param[in] tile_height hauteur en pixel de la tuile
     * \param[in] contexte de stockage (fichier, objet ceph ou objet swift)
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create a Rok4Image object, for writting
     * \details All informations have to be provided to be written in the TIFF header.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] name image's name
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] sample_format samples' format
     * \param[in] photometric data photometric
     * \param[in] compression data compression
     * \param[in] tile_width tile's pixel width
     * \param[in] tile_height tile's pixel height
     * \param[in] context storage context (file, ceph object or swift object)
     * \return a Rok4Image object pointer, NULL if error
     */
    static Rok4Image* create_to_write (
        std::string name, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
        SampleFormat::eSampleFormat sample_format, Photometric::ePhotometric photometric,
        Compression::eCompression compression, int tile_width, int tile_height, Context* context
    );


    /** \~french
     * \brief Crée un objet Rok4Image vecteur, pour l'écriture
     ** \~english
     * \brief Create a vecteor Rok4Image object, for writting
     */
    static Rok4Image* create_to_write (
        std::string name, int tilePerWidth, int tilePerHeight, Context* context
    );
};



