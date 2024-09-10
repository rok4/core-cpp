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

#include "compressors/LzwCompressor.h"

#define M_CLR    uint16_t(256)          // clear table marker 
#define M_EOD    uint16_t(257)          // end-of-data marker 

#include <cstddef>
#include <cstdlib>
#include <cstring>

LzwCompressor::LzwCompressor()
{
    //init Vector
    dict = std::vector<LzwWord>(258, LzwWord());
    dict.reserve(512);
    max_bit=12;
    max_code = 512;
    bit_size = 9;
    next_code = 258;
    last_code = 0;
    first_pass = true;
    buffer = 0;
    n_write_bits = 0;
}


void LzwCompressor::clear_dict()
{
    dict.clear();
    //init Vector
    dict = std::vector<LzwWord>(258, LzwWord());
    dict.reserve(512);
    next_code=258;
    max_code= 512;
    bit_size=9;
}

void LzwCompressor::write_bits(uint16_t lzwCode, uint8_t* out, size_t& outPos
)
{
    buffer = ( buffer << bit_size ) | lzwCode; // put lzwCode in the write buffer
    n_write_bits += bit_size;
    while (n_write_bits >=8) { // Write 8bit of Data
        n_write_bits -= 8;
        out[outPos++] = buffer >> n_write_bits;
        buffer = buffer & (1 << n_write_bits ) - 1;
    }
}


uint8_t* LzwCompressor::encode(const uint8_t* in, size_t inSize, size_t& outSize)
{
    size_t outBufferSize = inSize*2;
    size_t outPos = 0;
    uint8_t* out = new uint8_t[outBufferSize];

    uint8_t character=0;
    uint16_t next_code=0;
    if (first_pass && inSize) {
        //Initialize with first character
        last_code = *(in++);
        inSize--;
        first_pass = false;
        write_bits(M_CLR,out, outPos);
    }

    while (inSize) {
        character= *(in++);
        inSize--;
        if ((next_code = dict.at(last_code).next_value[character])) { // input already in dictionary waiting for new character
            last_code = next_code;
        } else { // Write Code and append to dictionary
            if (outPos + 2 > outBufferSize) {
                uint8_t* tmp_buffer = new uint8_t[(outBufferSize*2)];
                if (tmp_buffer) { // Enlarge your Buffer
                    memset(tmp_buffer+outBufferSize ,0,outBufferSize);
                    memcpy(tmp_buffer, out, outBufferSize);
                    delete[] out;
                    out = tmp_buffer;
                    outBufferSize *=2;
                } else { //Allocation error
                    outSize = 0;
                    return NULL;
                }
            }
            write_bits(last_code,out, outPos); // put LastCode in the write buffer
            dict[last_code].next_value[character]=dict.size();
            dict.push_back(LzwWord());
            if (dict.size() == max_code) {
                if (bit_size < max_bit) { //Extend
                    bit_size++;
                    max_code*=2;
                    dict.reserve(max_code);
                } else { // Clear Dict
                    write_bits(M_CLR,out, outPos);
                    clear_dict();
                }
            }
            last_code = character;

        }

    }
    write_bits(last_code,out, outPos);
    //Should be triggered at the end
    write_bits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        write_bits(buffer,out, outPos);
    }
    outSize = outPos;
    return out;
}

uint8_t* LzwCompressor::stream_encode(const uint8_t* in, size_t inSize, uint8_t* out, size_t& outSize)
{
    size_t outBufferSize = outSize;
    size_t outPos = 0;
    uint8_t character=0;
    uint16_t next_code=0;
    if (first_pass && inSize) {
        //Initialize with first character
        last_code = *(in++);
        inSize--;
        first_pass = false;
        write_bits(M_CLR,out, outPos);
    }

    while (inSize) {
        character= *(in++);
        inSize--;
        if ((next_code = dict.at(last_code).next_value[character])) { // input already in dictionary waiting for new character
            last_code = next_code;
        } else { // Write Code and append to dictionary
            write_bits(last_code,out, outPos); // put LastCode in the write buffer
            dict[last_code].next_value[character]=dict.size();
            dict.push_back(LzwWord());
            if (dict.size() == max_code) {
                if (bit_size < max_bit) { //Extend
                    bit_size++;
                    max_code*=2;
                    //dict.reserve(max_code);
                } else { // Clear Dict
                    write_bits(M_CLR,out, outPos);
                    clear_dict();
                }
            }
            last_code = character;
            if (outPos+3 > outBufferSize) {//Buffer too small
                outSize = outPos;
                return (uint8_t*) in;
            }
        }

    }
    /*write_bits(last_code,out, outPos);
    //Should be triggered at the end
    write_bits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        write_bits(buffer,out, outPos);
    }*/
    outSize = outPos;
    return out;
}

void LzwCompressor::streamEnd(uint8_t* out, size_t& outPos)
{
    write_bits(last_code,out, outPos);
    //Should be triggered at the end
    write_bits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        write_bits(buffer,out, outPos);
    }

}


uint8_t* LzwCompressor::encode_alt(const uint8_t* in, size_t inSize, size_t& outSize)
{
    outSize = 20;
    size_t outPos = outSize;
    size_t outBufferPos = 0;
    uint8_t* outBuffer = new uint8_t[outSize];
    memset(outBuffer ,0,outSize);
    uint8_t* out = outBuffer;
    uint8_t* oldout = out;
    uint8_t* inBuffer=(uint8_t*) in;
    out = stream_encode(in, inSize, out, outPos);
    while (out != oldout ) {

        uint8_t* tmp_buffer = new uint8_t[(outSize*2)];
        if (tmp_buffer) { // Enlarge your Buffer
            memset(tmp_buffer+outSize ,0,outSize);
            memcpy(tmp_buffer, outBuffer, outSize);
            delete[] outBuffer;
            outBuffer = tmp_buffer;
            tmp_buffer = NULL;
            outSize *=2;
        } else { //Allocation error
            outSize = 0;
            return NULL;
        }
        inBuffer = out;
        size_t inPos = (inBuffer - in);
        outBufferPos += outPos;
        oldout = outBuffer + outBufferPos;
        outPos = outSize - outPos;
        out = stream_encode(inBuffer, inSize - inPos, oldout, outPos);
    }
    outSize= outBufferPos + outPos;
    streamEnd(out,outSize);
    return out;
}


LzwCompressor::~LzwCompressor()
{

}


