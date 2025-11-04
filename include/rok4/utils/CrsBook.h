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
 * \file CrsBook.h
 ** \~french
 * \brief Définition de la classe CrsBook
 ** \~english
 * \brief Define classe CrsBook
 */

#pragma once

#include <boost/log/trivial.hpp>
#include <map>
#include <string.h>
#include <mutex>

#include "rok4/utils/CRS.h"
/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un annuaire de CRS
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class CrsBook {

private:

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    CrsBook();

    /**
     * \~french \brief Annuaire de styles
     * \details La clé est l'identifiant du style
     * \~english \brief Book of styles
     * \details Key is a the style identifier
     */
    static std::map<std::string,CRS*> book;

    /**
     * \~french \brief Exclusion mutuelle
     * \details Pour éviter les modifications concurrentes du cache de CRS
     * \~english \brief Mutual exclusion
     * \details To avoid concurrent CRS cache updates
     */
    static std::mutex mtx;

public:


    /**
     * \~french
     * \brief Retourne le CRS d'après son identifiant (code de requête)
     * \param[in] id Identifiant du CRS voulu

     * \brief Return the style according to its identifier
     * \param[in] id Wanted style identifier
     */
    static CRS* get_crs(std::string id);

    /**
     * \~french \brief Nettoie tous les CRS dans l'annuaire et le vide
     * \~english \brief Clean all CRS objects in the book and empty it
     */
    static void clean_crss ();

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~CrsBook();

};


