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

#include "compressors/LzwUncompressor.h"

#include <cstddef>
#include <cstring>
#include <list>
#include <iostream>

#define M_CLR    256          // clear table marker 
#define M_EOD    257          // end-of-data marker 
#define BUFFER_SIZE 256*256*4 // Default tile Size

LzwUncompressor::LzwUncompressor(uint8_t max_bit) : max_bit(max_bit) {
    bit_size=9;
    max_code=512;

    dict.reserve(max_code);
    for ( int i = 0; i < 255; ++i) {
        lzwWord word;
        word.push_back(i);
        dict.push_back(word);;
    }
    dict.push_back(lzwWord());
    dict.push_back(lzwWord());


    first_pass=true;
    buffer =0;
    n_read_bits = 0;
    last_char=0;
}


void LzwUncompressor::clear_dict() {
    dict.clear();
    dict.reserve(max_code);
    for ( int i = 0; i < 256; ++i) {
        lzwWord word;
        word.push_back(i);
        dict.push_back(word);;
    }
    dict.push_back(lzwWord());
    dict.push_back(lzwWord());

    bit_size=9;
    max_code=512;
    last_code=256;
    last_char=0;
    first_pass = true;
}

/** \warning Accès à un emplacement dans dict impossible. Des sortie ont été mises, pour pointer le problème. Le plantage n'est pas systématique pour une même image.
 ** \warning Performance de décodage assez mauvaises.
 */
uint8_t* LzwUncompressor::decode ( const uint8_t* in, size_t inSize, size_t& outPos )
{
    size_t outSize= (outPos?outPos:BUFFER_SIZE);
    uint8_t* out = new uint8_t[outSize];
    outPos=0;
    lzwWord outString = lzwWord();
    //Initialization

    while (inSize) {
        while (first_pass) {
            while ( n_read_bits < bit_size) {
                if ( inSize > 0) {
                    buffer = (buffer << 8) | *(in++);
                    n_read_bits += 8;
                } else { // Not enough data in the current buffer. Return current state
                    return out;
                }
            }

            n_read_bits -= bit_size;
            // Extract BitSize bits from buffer
            code = buffer >> n_read_bits;
            // Remove extracted code from buffer
            buffer = buffer & (1 << n_read_bits) - 1;
            //Test Code
            if (code == M_EOD ) { //End of Data
                return out;
            }
            if ( code == M_CLR ) { // Reset Dictionary
                this->clear_dict();
            } else {
                out[outPos++] = code;
                last_code = code;
                last_char = code;
                first_pass = false;
            }
        }
        //Read enough data from input stream
        while ( n_read_bits < bit_size) {
            if ( inSize > 0) {
                buffer = (buffer << 8) | *(in++);
                n_read_bits += 8;
                inSize--;
            } else { // Not enough data in the current buffer. Return current state
                return out;
            }
        }

        n_read_bits -= bit_size;
        // Extract BitSize bits from buffer
        code = buffer >> n_read_bits;
        // Remove extracted code from buffer
        buffer = buffer & (1 << n_read_bits) - 1;
        //Test Code
        if (code == M_EOD ) { //End of Data
            return out;
        }
        if ( code == M_CLR ) { // Reset Dictionary
            this->clear_dict();
        } else {
            if (code > dict.size() - 1) { // Code Not found
                //itCode = dict.find(last_code);
                if (last_code >= dict.size()) std::cout << "1 last_code = " << last_code << " et dict size = " << dict.size() << std::endl;
                lzwWord oldstring = dict.at(last_code);
                outString.assign(oldstring.begin(),oldstring.end());
                outString.push_back(last_char);
            } else { // Code found get Value
                outString.assign(dict.at(code).begin(),dict.at(code).end());
            }
            last_char = *(outString.begin());

            for (lzwWord::iterator it = outString.begin(); it != outString.end(); it++) {
                if (outPos >= outSize) {
                    uint8_t* tmp_buffer = new uint8_t[(outSize*2)];
                    if (tmp_buffer) { // Enlarge your Buffer
                        memset(tmp_buffer+outSize ,0,outSize);
                        memcpy(tmp_buffer, out, outSize);
                        delete[] out;
                        out = tmp_buffer;
                        outSize *=2;
                    } else { //Allocation error
                        outPos = 0;
                        return NULL;
                    }
                }
                out[outPos++]= *it;
            }

            if (last_code >= dict.size()) std::cout << "2 last_code = " << last_code << " et dict size = " << dict.size() << std::endl;
            lzwWord newEntry = dict.at(last_code);
            newEntry.push_back(last_char);
            dict.push_back(newEntry);
            last_code = code;
            //Dictionary need to be extended or reseted
            if (dict.size() == max_code -1) {
                //Extend
                if (bit_size < max_bit) {
                    bit_size++;
                    max_code*=2;
                    dict.reserve(max_code);
                }
                // else : the next code must be M_CLR is written in max_bit bit
            }
        }

    }

    return out;
}

LzwUncompressor::~LzwUncompressor()
{
    dict.clear();
}




