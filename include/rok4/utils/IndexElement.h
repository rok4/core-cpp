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
 * \file IndexElement.h
 ** \~french
 * \brief Définition de la classe IndexElement
 ** \~english
 * \brief Define classe IndexElement
 */

#pragma once

#include <boost/log/trivial.hpp>
#include <vector>
#include <string.h>

#include "rok4/storage/Context.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Élément du cache des index de dalle
 */
class IndexElement {
friend class IndexCache;

protected:
    /**
     * \~french \brief Date d'enregistrement dans le cache
     * \~english \brief Cache date
     */
    std::time_t date;
    /**
     * \~french \brief Clé sous laquelle l'élément est enregistré dans le cache
     * \~english \brief Cache key
     */
    std::string key;
    /**
     * \~french \brief Nom de la dalle dans laquelle lire la donnée
     * \~english \brief Data slab to read
     */
    std::string name;
    /**
     * \~french \brief Contexte de stockage de la dalle de donnée
     * \~english \brief Data slab storage context
     */
    Context* context;
    /**
     * \~french \brief Offsets des tuiles dans la dalle
     * \~english \brief Tiles' offsets
     */
    std::vector<uint32_t> offsets;
    /**
     * \~french \brief Tailles des tuiles dans la dalle
     * \~english \brief Tiles' sizes
     */
    std::vector<uint32_t> sizes;

    /** \~french
     * \brief Constructeur
     * \param[in] k clé d'enregistrement dans le cache
     * \param[in] s nom de la dalle
     * \param[in] c contexte de stockage de la dalle
     * \param[in] tiles_number nombre de tuiles dans la dalles
     * \param[in] os offsets bruts des tuiles 
     * \param[in] ss tailles brutes des tuiles
     ** \~english
     * \brief Constructor
     * \param[in] k cache key
     * \param[in] s data slab name
     * \param[in] c data slab storage context
     * \param[in] tiles_number tiles number
     * \param[in] os raw tiles' offsets
     * \param[in] ss raw tiles' sizes
     */
    IndexElement(std::string k, Context* c, std::string n, int tiles_number, uint8_t* os, uint8_t* ss);
};