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
 * \file IndexCache.h
 ** \~french
 * \brief Définition de la classes IndexCache
 ** \~english
 * \brief Define classe IndexCache
 */

#pragma once

#include <stdint.h>// pour uint8_t
#include <boost/log/trivial.hpp>
#include <map>
#include <list>
#include <unordered_map>
#include <string.h>
#include <thread>
#include <mutex>

#include "rok4/utils/Cache.h"
#include "rok4/storage/Context.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un cache des index de dalle
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class IndexCache {  

private:

    /**
     * \~french \brief Liste des éléments en cache
     * \~english \brief Cache elements list
     */
    static std::list<IndexElement *> cache;
    /**
     * \~french \brief Map d'index des éléments du cache
     * \~english \brief Cache element index map
     */
    static std::unordered_map<std::string, std::list<IndexElement *>::iterator> map;
    /**
     * \~french \brief Taille du cache en nombre d'élément
     * \details 100 par défaut
     * \~english \brief Cache size
     * \details Default value : 100
     */
    static int size;
    /**
     * \~french \brief Durée de validité en seconde d'un élément du cache
     * \details 300 par défaut (5 minutes)
     * \~english \brief Cache element's validity period, in seconds
     * \details Default value : 300 (5 minutes)
     */
    static int validity;

    /**
     * \~french \brief Exclusion mutuelle
     * \details Pour éviter les modifications concurrentes du cache des index
     * \~english \brief Mutual exclusion
     * \details To avoid concurrent index cache updates
     */
    static std::mutex mtx;

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    IndexCache();

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~IndexCache();

    /** \~french
     * \brief Définit la taille du cache
     * \param[in] s taille du cache
     ** \~english
     * \brief Define cache size
     * \param[in] s cache size
     */
    static void setCacheSize(int s);

    /** \~french
     * \brief Définit la durée de validité
     * \param[in] s durée de validité du cache, en secondes
     ** \~english
     * \brief Define cache validity
     * \param[in] s cache validity, in seconds
     */
    static void set_validity(int v);

    /** \~french
     * \brief Ajoute un élément au cache
     * \param[in] origin_slab_name nom d'interrogation de la dalle
     * \param[in] data_context contexte de stockage de la dalle de donnée
     * \param[in] data_slab_name nom de la dalle réelle contenant les données
     * \param[in] tiles_number nombre de tuiles dans la dalles
     * \param[in] os offsets bruts des tuiles 
     * \param[in] ss tailles brutes des tuiles
     ** \~english
     * \brief Add element to cache
     * \param[in] origin_slab_name Slab request name
     * \param[in] data_context data slab storage context
     * \param[in] data_slab_name data slab name
     * \param[in] tiles_number tiles number
     * \param[in] os raw tiles' offsets
     * \param[in] ss raw tiles' sizes
     */
    static void add_slab_infos(std::string origin_slab_name, Context* data_context, std::string data_slab_name, int tiles_number, uint8_t* offsets, uint8_t* sizes);

    /** \~french
     * \brief Demande un élément du cache
     * \param[in] key nom d'interrogation de la dalle
     * \param[in] tile_number numéro de la tuile voulue
     * \param[out] data_context contexte de stockage de la dalle de donnée
     * \param[out] data_slab_name nom de la dalle contenant les données
     * \param[out] offset offset de la tuile
     * \param[out] size taille de la tuile
     ** \~english
     * \brief Ask element from cache
     * \param[in] key Slab request name
     * \param[in] tile_number tile indice
     * \param[out] data_context data slab storage context
     * \param[out] data_slab_name Real data slab
     * \param[out] offset tile's offset
     * \param[out] size tile's size
     */
    static bool get_slab_infos(std::string key, int tile_number, Context** data_context, std::string* data_slab_name, uint32_t* offset, uint32_t* size);

    /**
     * \~french \brief Nettoie tous les objets dans le cache
     * \~english \brief Clean all element from the cache
     */
    static void clean_indexes ();
};