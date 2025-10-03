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

#include <boost/log/trivial.hpp>
#include <map>
#include <proj.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un pool de contextes Proj
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class ProjPool {  

private:

    /**
     * \~french \brief Annuaire des contextes Proj
     * \details La clé est l'identifiant du thread
     * \~english \brief Proj contexts book
     * \details Key is the thread's ID
     */
    static std::map<pthread_t, PJ_CONTEXT*> pool;

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    ProjPool();

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ProjPool();

    /**
     * \~french \brief Retourne un contexte Proj propre au thread appelant
     * \details Si il n'existe pas encore de contexte Proj pour ce tread, on le crée et on l'initialise
     * \~english \brief Get the Proj context specific to the calling thread
     * \details If Proj context doesn't exist for this thread, it is created and initialized
     */
    static PJ_CONTEXT* get_proj_env();

    /**
     * \~french \brief Affiche le nombre de contextes proj dans l'annuaire
     * \~english \brief Print the number of proj contexts in the book
     */
    static void print_projs_count ();

    /**
     * \~french \brief Nettoie tous les contextes proj dans l'annuaire et le vide
     * \~english \brief Clean all proj objects in the book and empty it
     */
    static void clean_projs ();
};
