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

#ifndef ASPECTIMAGE_H
#define ASPECTIMAGE_H

#include <string>

#include "rok4/image/Image.h"
#include "rok4/style/Aspect.h"


class AspectImage : public Image {

private:

    /** \~french
    * \brief Image d'origine utilisée pour calculer l'exposition
    ** \~english
    * \brief Origin image used to compute the aspect
    */
    Image* source_image;

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
    * \brief Configuration de l'aspect
    ** \~english
    * \brief Aspect configuration
    */
    Aspect* aspect;

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    template<typename T>
    int _getline ( T* buffer, int line );

public:

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int get_line ( float* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int get_line ( uint8_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int get_line ( uint16_t* buffer, int line );

    /** \~french
    * \brief Constructeur
    ** \~english
    * \brief Construtor
    */
    AspectImage ( Image* image, Aspect* asp);

    /** \~french
    * \brief Destructeur
    ** \~english
    * \brief Destructor
    */
    virtual ~AspectImage();

    /** \~french
     * \brief Sortie des informations sur l'image estompée
     ** \~english
     * \brief Estompage image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ AspectImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Algo = " << aspect->algo ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- min Slope = " << aspect->min_slope ;
        
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

};

#endif // ASPECTIMAGE_H