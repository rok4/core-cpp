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

#include "rok4/utils/Configuration.h"

#include <stdint.h>
#include <vector>
#include <map>
#include <stddef.h>

class Colour {
public:
    Colour ( uint8_t r=0, uint8_t g=0,uint8_t b=0, int a=0 );

    uint8_t r;
    uint8_t g;
    uint8_t b;
    int a;
    ~Colour();
    bool operator== ( const Colour& other ) const;
    bool operator!= ( const Colour& other ) const;
};



class Palette : public Configuration {
private:
    size_t png_palette_size;
    uint8_t* png_palette;
    bool png_palette_initialized;
    std::map<double,Colour> colours_map;
    bool rgb_continuous;
    bool alpha_continuous;
    bool no_alpha;

public:
    Palette();
    Palette ( json11::Json doc );

    Palette & operator= ( const Palette& pal );
    bool operator== ( const Palette& other ) const;
    bool operator!= ( const Palette& other ) const;
    virtual ~Palette();
    size_t get_png_palette_size();

    void build_png_palette();

    uint8_t* get_png_palette();
    std::map<double,Colour>* get_colours_map() {
        return &colours_map;
    }
    bool is_no_alpha() {
        return no_alpha;
    }
    bool is_empty() {
        return colours_map.empty();
    }
    Colour get_colour ( double index );

};



