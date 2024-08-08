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

#include "style/Palette.h"
#include <stdint.h>
#include "zlib.h"
#include <string.h>
#include "byteswap.h"
#include <boost/log/trivial.hpp>

Colour::Colour ( uint8_t r, uint8_t g, uint8_t b, int a ) : r ( r ), g ( g ), b ( b ), a ( a ) {

}

Colour::~Colour() {

}

bool Colour::operator== ( const Colour& other ) const {
    if ( this->r != other.r )
        return false;
    if ( this->g != other.g )
        return false;
    if ( this->b != other.b )
        return false;
    if ( this->a != other.a )
        return false;
    return true;
}

bool Colour::operator!= ( const Colour& other ) const {
    return ! ( *this == other );
}

Palette::Palette() : png_palette_initialized ( false ), rgb_continuous ( false ), alpha_continuous ( false ), no_alpha( false ) {
    png_palette_size = 0;
    png_palette = NULL;
}

Palette::Palette ( json11::Json doc ) : Configuration(), png_palette_size ( 0 ) ,png_palette ( NULL ) ,png_palette_initialized ( false ) {

    if (! doc.is_object()) {
        BOOST_LOG_TRIVIAL(warning) << "Wrong format for palette, palette ignored";
        return;
    }

    if (doc["rgb_continuous"].is_bool()) {
        rgb_continuous = doc["rgb_continuous"].bool_value();
    } else {
        rgb_continuous = false;
    }

    if (doc["alpha_continuous"].is_bool()) {
        alpha_continuous = doc["alpha_continuous"].bool_value();
    } else {
        alpha_continuous = false;
    }

    if (doc["no_alpha"].is_bool()) {
        no_alpha = doc["no_alpha"].bool_value();
    } else {
        no_alpha = false;
    }

    if (doc["colours"].is_array()) {
        for (json11::Json c : doc["colours"].array_items()) {
            if (c.is_object()) {
                if (c["value"].is_number() && c["red"].is_number() && c["green"].is_number() && c["blue"].is_number() && c["alpha"].is_number()) {
                    colours_map.insert ( std::pair<double,Colour> (
                        c["value"].number_value(),
                        Colour ( c["red"].number_value(),c["green"].number_value(),c["blue"].number_value(),c["alpha"].number_value() ) 
                    ) );
                }
            }
        }
    }
}

void Palette::build_png_palette() {
    BOOST_LOG_TRIVIAL(debug) <<  "ColourMapSize " << colours_map.size() ;
    if ( !colours_map.empty() ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Palette PNG OK" ;
        std::vector<Colour> colours;
        for ( int k = 0; k < 256 ; ++k ) {
            Colour tmp = get_colour ( k );
            colours.push_back ( Colour ( tmp.r,tmp.g,tmp.b, ( tmp.a==-1? ( 255-k ) :tmp.a ) ) );
        }
        int numberColor = colours.size();


        png_palette_size = numberColor* 3 + 12; // Palette(Nombre de couleur* nombre de canaux + header) 
        if (!no_alpha){
            png_palette_size += numberColor +12; //Palette =+ Transparence(Nombre de couleur + header)
        }
        png_palette = new uint8_t[png_palette_size];
        memset ( png_palette,0,png_palette_size );
        // Définition de la taille de la palette
        uint32_t paletteLenght = 3 * numberColor;
        * ( ( uint32_t* ) ( png_palette ) ) = bswap_32 ( paletteLenght );
        png_palette[0] = 0;
        png_palette[1] = 0;
        png_palette[2] = 3;
        png_palette[3] = 0;
        png_palette[4] = 'P';
        png_palette[5] = 'L';
        png_palette[6] = 'T';
        png_palette[7] = 'E';

        if (!no_alpha){
            png_palette[paletteLenght+12] = 0;
            png_palette[paletteLenght+12+1] = 0;
            png_palette[paletteLenght+12+2] = 3;
            png_palette[paletteLenght+12+3] = 0;
            png_palette[paletteLenght+12+4] = 't';
            png_palette[paletteLenght+12+5] = 'R';
            png_palette[paletteLenght+12+6] = 'N';
            png_palette[paletteLenght+12+7] = 'S';
        }

        Colour tmp;
        for ( int i =0; i < numberColor; i++ ) {
            tmp = colours.at ( i );

            png_palette[3*i+8]  = tmp.r;
            png_palette[3*i+9]  = tmp.g;
            png_palette[3*i+10] = tmp.b;
            if (!no_alpha) {
                png_palette[paletteLenght+12+i+8] = tmp.a;
            }
        }
        uint32_t crcPLTE = crc32 ( 0, Z_NULL, 0 );
        crcPLTE = crc32 ( crcPLTE, png_palette + 4, paletteLenght+4 );
        * ( ( uint32_t* ) ( png_palette + paletteLenght + 8 ) ) = bswap_32 ( crcPLTE );

        if (!no_alpha){
            uint32_t crctRNS = crc32 ( 0, Z_NULL, 0 );
            crctRNS = crc32 ( crctRNS, png_palette+ paletteLenght+12 + 4, 4 + numberColor );
            * ( ( uint32_t* ) ( png_palette + paletteLenght+ 12 + 8 + numberColor ) ) = bswap_32 ( crctRNS );
            uint32_t trnsLenght = numberColor;
            * ( ( uint32_t* ) ( png_palette+paletteLenght+12 ) ) = bswap_32 ( trnsLenght );
        }
    } else {
        png_palette_size=0;
        png_palette=NULL;
        BOOST_LOG_TRIVIAL(debug) <<  "Palette incompatible avec le PNG" ;
    }
    png_palette_initialized = true;
}

size_t Palette::get_png_palette_size() {
    if ( !png_palette_initialized )
        build_png_palette();
    return png_palette_size;
}

uint8_t* Palette::get_png_palette() {
    if ( !png_palette_initialized )
        build_png_palette();
    return png_palette;
}

Palette& Palette::operator= ( const Palette& pal ) {
    if ( this != &pal ) {
        this->png_palette_initialized = pal.png_palette_initialized;
        this->png_palette_size = pal.png_palette_size;
        this->rgb_continuous = pal.rgb_continuous;
        this->alpha_continuous = pal.alpha_continuous;
        this->colours_map = pal.colours_map;
        this->no_alpha = pal.no_alpha;

        if ( this->png_palette_size !=0 ) {
            this->png_palette = new uint8_t[png_palette_size];
            memcpy ( png_palette,pal.png_palette,this->png_palette_size );
        } else {
            this->png_palette = NULL;
        }
    }
    return *this;
}

bool Palette::operator== ( const Palette& other ) const {
    if ( this->png_palette && other.png_palette ) {
        if ( this->png_palette_size != other.png_palette_size )
            return false;
        for ( size_t pos = this->png_palette_size -1; pos; --pos ) {
            if ( ! ( * ( this->png_palette+pos ) == * ( other.png_palette+pos ) ) )
                return false;


        }
    }
    if ( this->colours_map.size() != other.colours_map.size() )
        return false;
    std::map<double,Colour>::const_iterator i,j;
    for ( i = this->colours_map.begin(), j = other.colours_map.begin(); i != this->colours_map.end(); ++i, ++j ) {
        if ( i->first == j->first ) {
            if ( i->second != j->second ) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool Palette::operator!= ( const Palette& other ) const {
    return ! ( *this == other );
}


Palette::~Palette() {
    if ( png_palette )
        delete[] png_palette;
}

Colour Palette::get_colour ( double index ) {
    std::map<double,Colour>::const_iterator nearestValue = colours_map.upper_bound ( index );
    if ( nearestValue != colours_map.begin() ) {
        nearestValue--;
    }

    Colour tmp = nearestValue->second;
    std::map<double,Colour>::const_iterator nextValue = nearestValue;
    nextValue++;
    if ( nextValue != colours_map.end() ) { // Cas utile
        std::map<double,Colour>::const_iterator nextValue = nearestValue;
        nextValue++;
        if ( rgb_continuous ) {
            tmp.r = ( ( nextValue )->second.r - nearestValue->second.r ) / ( ( nextValue )->first - nearestValue->first ) * index +
                    ( ( nextValue )->first * nearestValue->second.r - nearestValue->first * ( nextValue )->second.r ) / ( ( nextValue )->first - nearestValue->first );
            tmp.g = ( ( nextValue )->second.g - nearestValue->second.g ) / ( ( nextValue )->first - nearestValue->first ) * index +
                    ( ( nextValue )->first * nearestValue->second.g - nearestValue->first * ( nextValue )->second.g ) / ( ( nextValue )->first - nearestValue->first );
            tmp.b = ( ( nextValue )->second.b - nearestValue->second.b ) / ( ( nextValue )->first - nearestValue->first ) * index +
                    ( ( nextValue )->first * nearestValue->second.b - nearestValue->first * ( nextValue )->second.b ) / ( ( nextValue )->first - nearestValue->first );
        }
        if ( alpha_continuous ) {
            tmp.a = ( ( nextValue )->second.a - nearestValue->second.a ) / ( ( nextValue )->first - nearestValue->first ) * index +
                    ( ( nextValue )->first * nearestValue->second.a - nearestValue->first * ( nextValue )->second.a ) / ( ( nextValue )->first - nearestValue->first );
        }
    }
    return tmp;
}


