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
 * \file TerrainrgbImage.h
 ** \~french
 * \brief D�finition de la classe TerrainrgbImage
 ** \~english
 * \brief Define class TerrainrgbImage
 */

#pragma once

#include "rok4/image/Image.h"
#include "rok4/image/EstompageImage.h"
#include "rok4/image/PaletteImage.h"
#include "rok4/image/PenteImage.h"
#include "rok4/image/AspectImage.h"
#include "rok4/image/TerrainrgbImage.h"
#include "rok4/style/Style.h"

class StyledImage : public Image
{
private:
    template <typename T>
    int _getline(T *buffer, int line);
    Image *source_image;
    Style *style;
    /** \~french
    * \brief Résolution de l'image en X, en mètre
    ** \~english
    * \brief Resolution of the image (X), in meter
    */
    float resxmeter;

    /** \~french
    * \brief Résolution de l'image en Y, en mètre
    ** \~english
    * \brief Resolution of the image (Y), in meter
    */
    float resymeter;

    /** \~french
    * \brief Nombre de ligne en mémoire
    ** \~english
    * \brief Memorize lines number
    */
    int memorized_source_lines;

    /** \~french
    * \brief Buffer contenant les lignes sources
    ** \~english
    * \brief Source lines memory buffer
    */
    float* source_lines_buffer;

    /** \~french
    * \brief Numéros des lignes en mémoire
    ** \~english
    * \brief Memorized lines indexes
    */
    int* source_lines;

    /** \~french
    * \brief Matrice de convolution
    ** \~english
    * \brief Convolution matrix
    */
    float matrix[9];

    /** \~french
    * \brief Résolution de l'image d'origine et donc finale
    ** \~english
    * \brief Resolution of the image
    */
    float resolution;

    /** \~french
    * \brief Booléen précisant l'utilisation de buffer pour les traitements multi-lignes
    ** \~english
    * \brief Booleen that indicate if buffers are used for multi-line processes
    */
    bool multi_line_buffer;

    StyledImage(Image* image, Style *style, int offset);

public:
    virtual int get_line(float *buffer, int line);
    virtual int get_line(uint16_t *buffer, int line);
    virtual int get_line(uint8_t *buffer, int line);
    

    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image stylisée et crée un objet StyledImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres. On vérifie la superposabilité des images sources.
     * \param[in] input_image image source
     * \param[in] input_style style source
     * \return un pointeur d'objet StyledImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate styled image components and create an StyledImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters. We check if source images are superimpose.
     * \param[in] input_image source images
     * \param[in] input_style nodata value
     * \return a StyledImage object pointer, NULL if error
     */
    static StyledImage* create ( Image* input_image, Style* input_style );

    virtual ~StyledImage();
    /** \~french
     * \brief Sortie des informations sur l'image reprojetée
     ** \~english
     * \brief Reprojected image description output
     */
    void print();
};
