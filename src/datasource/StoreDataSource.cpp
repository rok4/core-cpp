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
 * \file StoreDataSource.cpp
 ** \~french
 * \brief Implémentation des classes StoreDataSource
 * \details
 * \li StoreDataSource : Permet de lire de la donnée quelque soit le type de stockage
 ** \~english
 * \brief Implements classes StoreDataSource
 * \details
 * \li StoreDataSource : To read data, whatever the storage type
 */

#include "datasource/StoreDataSource.h"
#include <fcntl.h>
#include <boost/log/trivial.hpp>
#include <cstdio>
#include <errno.h>
#include "enums/Format.h"
#include "storage/Context.h"

StoreDataSource::StoreDataSource (std::string n, Context* c, const uint32_t o, const uint32_t s, std::string type, std::string encoding ) :
    name ( n ), context(c), offset(o), wanted_size(s), tile_indice(-1), tiles_number(-1), type (type), encoding( encoding )
{
    data = NULL;
    size = 0;
    already_tried = false;
}

StoreDataSource::StoreDataSource (const int tile_ind, const int tiles_nb, std::string n, Context* c, std::string type, std::string encoding ) :
    name ( n ), context(c), offset(0), wanted_size(0), tile_indice(tile_ind), tiles_number(tiles_nb), type (type), encoding( encoding )
{
    data = NULL;
    size = 0;
    already_tried = false;
}

/*
 * Fonction retournant les données de la tuile
 * Le fichier/objet ne doit etre lu qu une seule fois
 * Indique la taille de la tuile (inconnue a priori)
 */
const uint8_t* StoreDataSource::get_data ( size_t &tile_size ) {
    if ( already_tried) {
        tile_size = size;
        return data;
    }

    already_tried = true;

    // il se peut que le contexte d'origine n'existe pas ou ne soit pas connecté, auquel cas on sort directement sans donnée
    if (! context->is_connected()) {
        data = NULL;
        return NULL;
    }

    if (tile_indice == -1) {
        // On a directement la taille et l'offset
        data = new uint8_t[wanted_size];
        try {
            int read_size = context->read(data, offset, wanted_size, name);
            tile_size = read_size;
            size = tile_size;
        } catch (StorageException& err) {
            delete[] data;
            data = NULL;
            throw;
        }
    } else {
        // Nous n'avons pas les infos de taille et d'offset pour la tuile, nous allons devoir les récupérer lire
        // On va regarder si on n'a pas nos informations dans le cache

        std::string full_name = context->get_path(name);

        BOOST_LOG_TRIVIAL(debug) << "input slab " << full_name;
        
        if (! IndexCache::get_slab_infos(full_name, tile_indice, &context, &name, &offset, &wanted_size)) {
            // Rien de valide dans le cache, on va lire l'index de la dalle
            BOOST_LOG_TRIVIAL(debug) << "pas de cache";

            std::string original_full_name (full_name);
            std::string original_tray_name (context->get_tray());

            int header_index_size = ROK4_IMAGE_HEADER_SIZE + 2 * 4 * tiles_number;
            uint8_t* index_header = new uint8_t[header_index_size];
            int real_size;

            try {
                real_size = context->read(index_header, 0, header_index_size, name);
            } catch (StorageException& err) {
                delete[] index_header;
                throw;
            }

            if ( real_size < header_index_size ) {

                // On a lu moins que ce qu'on voulait : 
                //      - soit c'est une dalle symbolique, ce qu'on va confirmer via la signature
                //      - soit c'est une dalle cassée

                // Dans le cas d'un header de type objet lien, on verifie d'abord que la signature concernée est bien presente dans le header de l'objet
                if ( real_size < ROK4_SYMLINK_SIGNATURE_SIZE || strncmp((char*) index_header, ROK4_SYMLINK_SIGNATURE, ROK4_SYMLINK_SIGNATURE_SIZE) != 0 ) {
                    BOOST_LOG_TRIVIAL(error) << "Read data in " << full_name  << " is neither an header and an index (too small) nor a link (no signature)";
                    delete[] index_header;
                    return NULL;
                }

                // On est dans le cas d'un objet symbolique

                BOOST_LOG_TRIVIAL(debug) << "dalle symbolique";

                char tmp_name[real_size-ROK4_SYMLINK_SIGNATURE_SIZE+1];
                memcpy((uint8_t*) tmp_name, index_header+ROK4_SYMLINK_SIGNATURE_SIZE,real_size-ROK4_SYMLINK_SIGNATURE_SIZE);
                tmp_name[real_size-ROK4_SYMLINK_SIGNATURE_SIZE] = '\0';
                full_name = std::string (tmp_name);
                name = full_name;


                BOOST_LOG_TRIVIAL(debug) << " -> " << full_name;

                if (context->get_type() != ContextType::FILECONTEXT) {
                    // Dans le cas du stockage objet, on sépare le nom du contenant du nom de l'objet
                    std::stringstream ss(full_name);
                    std::string token;
                    char delim = '/';
                    std::getline(ss, token, delim);
                    std::string tray_name = token;
                    name.erase(0, tray_name.length() + 1);

                    if (original_tray_name != tray_name) {
                        // Récupération ou ajout du nouveau contexte de stockage
                        // On reprécise le contexte d'origine, pour utiliser le même cluster en cas S3
                        context = StoragePool::get_context(context->get_type(), tray_name, context);
                        // Problème lors de l'ajout ou de la récupération de ce contexte de stockage
                        if (context == NULL) {
                            data = NULL;
                            return NULL;
                        }
                    }
                }

                BOOST_LOG_TRIVIAL(debug) <<  "Symbolic slab detected : " << original_full_name << " -> " << full_name ;

                try {
                    real_size = context->read(index_header, 0, header_index_size, name);
                } catch (StorageException& err) {
                    delete[] index_header;
                    throw;
                }
                
                if ( real_size < header_index_size ) {
                    BOOST_LOG_TRIVIAL(error) << "Read data in slab " << full_name << " (referenced by " << original_full_name << ") is too small to be an header and an index";
                    delete[] index_header;
                    return NULL;
                }

            }

            IndexCache::add_slab_infos(original_full_name, context, name, tiles_number, index_header + ROK4_IMAGE_HEADER_SIZE, index_header + ROK4_IMAGE_HEADER_SIZE + 4 * tiles_number);
            offset = *((uint32_t*) (index_header + ROK4_IMAGE_HEADER_SIZE + 4 * tile_indice ));
            wanted_size = *((uint32_t*) (index_header + ROK4_IMAGE_HEADER_SIZE + 4 * tiles_number + 4 * tile_indice ));
            delete[] index_header;
        }

        if ( wanted_size == 0 ) {
            BOOST_LOG_TRIVIAL(debug) <<  "Tuile non présente dans la dalle (taille nulle) " << full_name  ;
            data = NULL;
            return NULL;
        }

        // Lecture de la tuile
        data = new uint8_t[wanted_size];

        try {
            context->read(data, offset, wanted_size, name);
        } catch (StorageException& err) {
            delete[] data;
            data = NULL;
            throw;
        }

        tile_size = wanted_size;
        size = wanted_size;
    }

    return data;
}
