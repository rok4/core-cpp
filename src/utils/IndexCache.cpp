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
 * \file IndexCache.cpp
 ** \~french
 * \brief Implémentation de la classe IndexCache
 ** \~english
 * \brief Implements classe IndexCache
 */

#include "rok4/utils/IndexCache.h"

IndexCache::IndexCache() {

}

IndexCache::~IndexCache() {

}

void IndexCache::setCacheSize(int s) {
    size = s;
}

void IndexCache::set_validity(int v) {
    validity = v;
}

void IndexCache::add_slab_infos(std::string origin_slab_name, Context *data_context, std::string data_slab_name, int tiles_number, uint8_t *offsets, uint8_t *sizes) {
    // On utilise le nom original de la dalle (celle à lire a priori) comme clé dans la map
    // Potentiellement ce nom est différent de la vraie dalle contenant la donnée (dans le cas d'une dalle symbolique)
    // mais c'est via cette dalle symbolique que la donnée est a priori requêtée
    // donc c'est ce nom qu'on utilisera pour l'interrogation du cache

    mtx.lock();

    IndexElement *elem = new IndexElement(origin_slab_name, data_context, data_slab_name, tiles_number, offsets, sizes);

    // L'information n'est a priori pas dans le cache car
    // Soit elle était dans le cache et valide, alors on aurait utilisé ce cache et on n'aurait pas fait appel à cet ajout
    // Soit elle était dans le cache mais obsolète, alors on l'aurait déjà supprimé du cache

    if (cache.size() == size)
    {
        // On récupère le dernier élément du cache
        IndexElement *last = cache.back();
        // On le vire du cache
        cache.pop_back();
        // On le déréférence de la map d'accès
        map.erase(last->key);
        delete last;
    }

    // update reference
    cache.push_front(elem);
    map[origin_slab_name] = cache.begin();

    mtx.unlock();
}

bool IndexCache::get_slab_infos(std::string key, int tile_number, Context **data_context, std::string *data_slab_name, uint32_t *offset, uint32_t *size) {
    std::unordered_map<std::string, std::list<IndexElement *>::iterator>::iterator it = map.find(key);
    if (it == map.end())
    {
        return false;
    }
    else
    {
        // Gestion de la péremption du cache (une heure max)
        std::time_t now = std::time(NULL);
        if (now - (*(it->second))->date > validity)
        {
            mtx.lock();

            // on le cherche à nouveau pour vérifier qu'il n'a pas déjà été supprimé par un thread concurrent
            it = map.find(key);

            if (it != map.end())
            {
                delete *(it->second);
                cache.erase(it->second);
                map.erase(it);
            }

            mtx.unlock();

            return false;
        }

        *data_slab_name = (*(it->second))->name;
        *data_context = (*(it->second))->context;
        *offset = (*(it->second))->offsets.at(tile_number);
        *size = (*(it->second))->sizes.at(tile_number);

        // On fait le choix de ne pas remettre en tête du cache cet élément, même s'il vient d'être accéder
        // C'est parce qu'en plus d'avoir une taille limite, les élément cachés ont une date de péromption
        // Il serait donc dommage de remettre en avant (donc plus loin d'une suppression par taille de cache atteinte)
        // un élément qui va de toute manière finir par être obsolète.
        // C'est une différence avec une cache LRU classique

        return true;
    }
}

void IndexCache::clean_indexes() {
    mtx.lock();
    std::list<IndexElement*>::iterator it;
    for (it = cache.begin(); it != cache.end(); ++it) {
        delete *it;
    }
    cache.clear();
    mtx.unlock();
}
