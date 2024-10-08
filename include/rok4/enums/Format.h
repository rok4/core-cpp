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
 * \file Format.h
 ** \~french
 * \brief Définition des namespaces Compression, SampleFormat, Photometric, ExtraSample et Rok4Format
 * \details
 * \li SampleFormat : gère les types de canaux acceptés par les classes d'Image
 * \li Compression : énumère et manipule les différentes compressions
 * \li Rok4Format : énumère et manipule les différents format de données ROK4
 * \li Photometric : énumère et manipule les différentes photométries
 * \li ExtraSample : énumère et manipule les différents type de canal supplémentaire
 ** \~english
 * \brief Define and the namespaces Compression, SampleFormat, Photometric, ExtraSample et Rok4Format
 * \details
 * \li SampleFormat : managed sample type accepted by Image classes
 * \li Compression : enumerate and managed different compressions
 * \li Rok4Format : enumerate and managed different ROK4 data formats
 * \li Photometric : enumerate and managed different photometrics
 * \li ExtraSample : enumerate and managed different extra sample types
 */

#pragma once

#include <string>
#include <stdint.h>

#define ROK4_IMAGE_HEADER_SIZE 2048
#define ROK4_SYMLINK_SIGNATURE_SIZE 8
#define ROK4_SYMLINK_SIGNATURE "SYMLINK#"
#define JPEG_BLOC_SIZE 16

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées au format de canal
 * \~english \brief Manage informations in connection with sample format
 */
namespace SampleFormat {
/**
 * \~french \brief Énumération des formats de canal disponibles
 * \~english \brief Available sample formats enumeration
 */
enum eSampleFormat {
    UNKNOWN = 0,
    UINT8 = 1,
    UINT16 = 2,
    FLOAT32 = 3
};

/**
 * \~french \brief Nombre de formats de canal disponibles
 * \~english \brief Number of sample formats compressions
 */
const int sampleformat_size = 3;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une format du canal de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la format du canal correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a sample formats enumeration member
 * \param[in] strComp string to convert
 * \return the binding sample format, UNKNOWN (0) if string is not recognized
 */
eSampleFormat from_string ( std::string sf );

/**
 * \~french \brief Conversion d'une format du canal vers une chaîne de caractères
 * \param[in] sf format du canal à convertir
 * \return la chaîne de caractère nommant la format du canal
 * \~english \brief Convert a sample format to a string
 * \param[in] sf sample format to convert
 * \return string namming the sample format
 */
std::string to_string ( eSampleFormat sf );

/**
 * \~french \brief Donne le nombre de bits par canal du format
 * \param[in] format format à connaître
 * \~english \brief Precise format's bits per sample
 * \param[in] format format to know
 */
int get_bits_per_sample ( eSampleFormat format );

}






/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la compression
 * \~english \brief Manage informations in connection with compression
 */
namespace Compression {
/**
 * \~french \brief Énumération des compressions disponibles
 * \~english \brief Available compressions enumeration
 */
enum eCompression {
    UNKNOWN = 0,
    NONE = 1,
    DEFLATE = 2,
    JPEG = 3,
    JPEG90 = 4,
    PNG = 5,
    LZW = 6,
    PACKBITS = 7,
    JPEG2000 = 8
};

/**
 * \~french \brief Nombre de compressions disponibles
 * \~english \brief Number of available compressions
 */
const int compression_size = 8;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une compression de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la compression correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a compressions enumeration member
 * \param[in] strComp string to convert
 * \return the binding compression, UNKNOWN (0) if string is not recognized
 */
eCompression from_string ( std::string strComp );

/**
 * \~french \brief Conversion d'une compression vers une chaîne de caractères
 * \param[in] comp compression à convertir
 * \return la chaîne de caractère nommant la compression
 * \~english \brief Convert a compression to a string
 * \param[in] comp compression to convert
 * \return string namming the compression
 */
std::string to_string ( eCompression comp );

}






/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la photométrie
 * \~english \brief Manage informations in connection with photometric
 */
namespace Photometric {
/**
 * \~french \brief Énumération des photométries disponibles
 * \~english \brief Available photometrics enumeration
 */
enum ePhotometric {
    UNKNOWN = 0,
    GRAY = 1,
    RGB = 2,
    PALETTE = 3,
    YCBCR = 4,
    MASK = 5
};

/**
 * \~french \brief Nombre de photométries disponibles
 * \~english \brief Number of photometrics compressions
 */
const int photometric_size = 5;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une photométrie de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la photométrie correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a photometrics enumeration member
 * \param[in] strComp string to convert
 * \return the binding photometric, UNKNOWN (0) if string is not recognized
 */
ePhotometric from_string ( std::string strPh );

/**
 * \~french \brief Conversion d'une photométrie vers une chaîne de caractères
 * \param[in] comp photométrie à convertir
 * \return la chaîne de caractère nommant la photométrie
 * \~english \brief Convert a photometric to a string
 * \param[in] comp photometric to convert
 * \return string namming the photometric
 */
std::string to_string ( ePhotometric ph );

}



/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées aux canaux supplémentaires
 * \~english \brief Manage informations in connection with extra samples
 */
namespace ExtraSample {
/**
 * \~french \brief Énumération des types de canal supplémentaire
 * \~english \brief Available extra samples type enumeration
 */
enum eExtraSample {
    UNKNOWN = 0,
    NONE = 1,
    ALPHA_ASSOC = 2,
    ALPHA_UNASSOC = 3
};

/**
 * \~french \brief Nombre de type de canal supplémentaire disponibles
 * \~english \brief Number of extra sample type compressions
 */
const int extraSample_size = 3;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers un type de canal supplémentaire de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return le type de canal supplémentaire correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a extra sample type enumeration member
 * \param[in] strComp string to convert
 * \return the binding extra sample type, UNKNOWN (0) if string is not recognized
 */
eExtraSample from_string ( std::string strES );

/**
 * \~french \brief Conversion d'un type de canal supplémentaire vers une chaîne de caractères
 * \param[in] comp type de canal supplémentaire à convertir
 * \return la chaîne de caractère nommant le type de canal supplémentaire
 * \~english \brief Convert a extra sample type to a string
 * \param[in] comp extra sample type to convert
 * \return string namming the extra sample type
 */
std::string to_string ( eExtraSample es );

}



/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées au format des données
 * \~english \brief Manage informations in connection with data format
 */
namespace Rok4Format {

/**
 * \~french \brief Énumération des formats d'images disponibles
 * \~english \brief Available images formats enumeration
 */
enum eFormat {
    UNKNOWN = 0,

    TIFF_RAW_UINT8 = 1,
    TIFF_JPG_UINT8 = 2,
    TIFF_JPG90_UINT8 = 3,
    TIFF_PNG_UINT8 = 4,
    TIFF_LZW_UINT8 = 5,
    TIFF_ZIP_UINT8 = 6,
    TIFF_PKB_UINT8 = 7,

    TIFF_RAW_FLOAT32 = 8,
    TIFF_LZW_FLOAT32 = 9,
    TIFF_ZIP_FLOAT32 = 10,
    TIFF_PKB_FLOAT32 = 11,

    TIFF_PBF_MVT = 12
};

/**
 * \~french \brief Nombre de formats disponibles
 * \~english \brief Number of available formats
 */
const int eformat_size = 12;

/**
 * \~french \brief Conversion d'une chaîne de caractère vers un format
 * \param[in] strFormat chaîne de caractère à convertir
 * \return le format correspondant, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a format
 * \param[in] strFormat string to convert
 * \return the binding format, UNKNOWN (0) if string is not recognized
 */
eFormat from_string ( std::string strFormat );


/**
 * \~french \brief Dit si un format est raster ou non
 * \param[in] format format à connaître
 * \~english \brief Say if a format is raster format
 * \param[in] format format to know
 */
bool is_raster ( eFormat format );

/**
 * \~french \brief Donne la compression du format
 * \param[in] format format à connaître
 * \~english \brief Precise format's compression
 * \param[in] format format to know
 */
Compression::eCompression get_compression ( eFormat format );

/**
 * \~french \brief Donne le format des canaux du format
 * \param[in] format format à connaître
 * \~english \brief Precise format's sample format
 * \param[in] format format to know
 */
SampleFormat::eSampleFormat get_sample_format ( eFormat format );

/**
 * \~french \brief Donne le nombre de bits par canal du format
 * \param[in] format format à connaître
 * \~english \brief Precise format's bits per sample
 * \param[in] format format to know
 */
int get_bits_per_sample ( eFormat format );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère
 * \param[in] format format à convertir
 * \return la chaîne de caractère nommant le format
 * \~english \brief Convert a format to a string
 * \param[in] format format to convert
 * \return string namming the format
 */
std::string to_string ( eFormat format );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère (type MIME)
 * \param[in] format format à convertir
 * \return type MIME du format
 * \~english \brief Convert a format to a string (type MIME)
 * \param[in] format format to convert
 * \return MIME type of the format
 */
std::string to_mime_type ( eFormat format );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère (format OGC API Tiles)
 * \param[in] format format à convertir
 * \return format OGC API Tiles du format
 * \~english \brief Convert a format to a string (format OGC API Tiles)
 * \param[in] format format to convert
 * \return format OGC API Tiles of the format
 */
std::string to_tiles_format ( eFormat format );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère (extension)
 * \param[in] format format à convertir
 * \return extension du format
 * \~english \brief Convert a format to a string (extension)
 * \param[in] format format to convert
 * \return extension of the format
 */
std::string to_extension ( eFormat format );

std::string to_encoding ( eFormat format );

}



