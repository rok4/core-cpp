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

#ifndef ESTOMPAGEIMAGE_H
#define ESTOMPAGEIMAGE_H

#include "rok4/image/Image.h"
#include "rok4/style/Estompage.h"

class EstompageImage : public Image {
private:
    /** \~french
    * \brief Image d'origine utilisée pour calculer l'estompage
    ** \~english
    * \brief Origin image used to compute the estompage
    */
    Image* origImage;

    /** \~french
    * \brief Nombre de ligne en mémoire
    ** \~english
    * \brief Memorize lines number
    */
    int memorizedOrigLines;

    /** \~french
    * \brief Buffer contenant les lignes sources
    ** \~english
    * \brief Source lines memory buffer
    */
    float* origLinesBuffer;

    /** \~french
    * \brief Numéros des lignes en mémoire
    ** \~english
    * \brief Memorized lines indexes
    */
    int* origLines;

    float zenith;
    float azimuth;
    float zFactor;

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
    * \brief Calcule la ligne
    ** \~english
    * \brief Process line
    */
    template<typename T>
    int _getline ( T* buffer, int line );

public:
    virtual int getline ( float* buffer, int line );
    virtual int getline ( uint8_t* buffer, int line );
    virtual int getline ( uint16_t* buffer, int line );
    EstompageImage (Image* image, Estompage* est);
    virtual ~EstompageImage();

    /** \~french
     * \brief Sortie des informations sur l'image estompée
     ** \~english
     * \brief Estompage image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ EstompageImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Zenith = " << zenith ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Azimuth = " << azimuth ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Z factor = " << zFactor ;
        
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }
};

#endif // ESTOMPAGEIMAGE_H