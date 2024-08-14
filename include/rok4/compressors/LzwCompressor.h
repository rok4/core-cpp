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

#ifndef LZWCOMPRESSOR_H
#define LZWCOMPRESSOR_H


#include <cstddef>
#include <climits>
#include <stdint.h>
#include <vector>
#include <string>
#include <list>

class LzwCompressor
{
    struct LzwWord 
    {
        uint16_t next_value[256];
    };
private:
    uint8_t max_bit;
    
    std::vector<LzwWord> dict;
    uint16_t max_code;
    uint8_t bit_size;
    uint16_t next_code;
    
    uint16_t last_code;
    
    uint32_t buffer;
    uint8_t n_write_bits;
    bool first_pass;
    void clear_dict();

    inline void write_bits(uint16_t lzwCode, uint8_t* out, size_t& outPos);
    uint8_t* encode_alt(const uint8_t * in, size_t inSize, size_t &outSize);
    uint8_t* stream_encode(const uint8_t * in, size_t inSize, uint8_t* outbuffer , size_t &outSize);
    void streamEnd(uint8_t* out, size_t& outSize);
    
public:
    LzwCompressor();
    uint8_t* encode(const uint8_t * in, size_t inSize, size_t &outSize);

    
    virtual ~LzwCompressor();
};

#endif // LZWCOMPRESSOR_H
