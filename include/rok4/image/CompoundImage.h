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

#pragma once

#include <vector>

#include "rok4/image/Image.h"

class CompoundImage : public Image {

private:

    static int compute_width ( std::vector<std::vector<Image*> > &source_images );
    static int compute_height ( std::vector<std::vector<Image*> > &source_images );
    static BoundingBox<double> compute_bbox ( std::vector<std::vector<Image*> > &source_images );

    std::vector<std::vector<Image*> > source_images;

    /** Indice y des tuiles courantes */
    int y;

    /** ligne correspondant au haut des tuiles courantes*/
    int top;

    template<typename T>
    inline int _getline ( T* buffer, int line );

public:

    /** D */
    int get_line ( uint8_t* buffer, int line );

    /** D */
    int get_line ( uint16_t* buffer, int line );
    
    /** D */
    int get_line ( float* buffer, int line );

    /** D */
    CompoundImage ( std::vector< std::vector<Image*> >& source_images );

    /** D */
    ~CompoundImage() {
        
        if ( ! is_mask ) {
            for ( int y = 0; y < source_images.size(); y++ )
                for ( int x = 0; x < source_images[y].size(); x++ )
                    delete source_images[y][x];
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image composée simple
     ** \~english
     * \brief Simple compounded image description output
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "------ CompoundImage -------" ;
        Image::print();
        BOOST_LOG_TRIVIAL(info) <<  "\t- Number of images = " ;
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- heightwise = " << source_images.size();
        BOOST_LOG_TRIVIAL(info) <<  "\t\t- widthwise = " << source_images.at(0).size();
    }

};


