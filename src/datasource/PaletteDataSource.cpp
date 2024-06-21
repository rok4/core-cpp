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

#include "datasource/PaletteDataSource.h"
#include "byteswap.h"
#include "zlib.h"
#include <string.h>

//TODO modification du header : 25 | Colour type  passage de 0 à 2
//TODO ajout de la palette

PaletteDataSource::PaletteDataSource ( DataSource *source_data, Palette* p ) : source_data ( source_data ) {
    palette = p;

    if ( source_data->get_type().compare ( "image/png" ) == 0 && palette != 0 && ! palette->is_empty() && palette->getPalettePNGSize() != 0 ) {
        // On récupère le contenu du fichier
        size_t tmp_size;
        const uint8_t* tmp = source_data->get_data ( tmp_size );
        data_size = tmp_size + palette->getPalettePNGSize() +1;
        size_t pos = 33;
        // Taille en sortie = taille en entrée +  3*256+12 (PLTE) + 256+12 (tRNS)
        data = new uint8_t[data_size];
        // Copie de l'entete
        memcpy ( data,tmp, pos );
        data[25] = 3; // mode palette
        //Mise à jour du crc du Header:
        uint32_t crch = crc32 ( 0, Z_NULL, 0 );
        crch = crc32 ( crch, data+8+4, 13+4 );
        * ( ( uint32_t* ) ( data+8+8+13 ) ) = bswap_32 ( crch );
        //Copie de la palette
        memcpy ( data+pos,palette->getPalettePNG(),palette->getPalettePNGSize() );
        pos += palette->getPalettePNGSize();
        //Copie des données
        memcpy ( data+pos, tmp +33, tmp_size - 33 );
    } else {
        // Si la datasource en entrée n'est pas de type image/png, que la palette est nulle ou vide on n'applique pas la palette
        // On retournera alors taille et donnée directement depuis la datasource en entrée
        data_size = 0;
        data = NULL;
    }

}


const uint8_t* PaletteDataSource::get_data ( size_t& size ) {
    if ( data != NULL ) {
        size = data_size;
        return data;
    } else {
        return source_data->get_data ( size );
    }
}

unsigned int PaletteDataSource::get_length ( ) {
    if ( data_size != 0 ) {
        return data_size;
    } else {
        return source_data->get_length();
    }
}

PaletteDataSource::~PaletteDataSource() {
    source_data->release_data();
    delete source_data;
    if ( data != NULL )
        delete[] data;
}


