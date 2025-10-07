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
 * \file StyleBook.h
 ** \~french
 * \brief Définition de la classe StyleBook
 ** \~english
 * \brief Define classe StyleBook
 */

#pragma once

#include <boost/log/trivial.hpp>
#include <map>
#include <vector>
#include <string.h>
#include <thread>
#include <mutex>

#include "rok4/style/Style.h"
#include "rok4/utils/Utils.h"

#define ROK4_STYLES_DIRECTORY "ROK4_STYLES_DIRECTORY"
#define ROK4_STYLES_NO_CACHE "ROK4_STYLES_NO_CACHE"


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un annuaire de styles
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class StyleBook {

private:

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    StyleBook();

    /**
     * \~french
     * \brief Répertoire de stockage des styles
     * \~english
     * \brief TMS storage directory
     */
    static std::string directory;

    /**
     * \~french \brief Annuaire de styles
     * \details La clé est l'identifiant du style
     * \~english \brief Book of styles
     * \details Key is a the style identifier
     */
    static std::map<std::string,Style*> book;

    /**
     * \~french \brief Corbeille de styles à supprimer
     * \~english \brief Styles trash to delete
     */
    static std::vector<Style*> trash;

    /**
     * \~french \brief Exclusion mutuelle
     * \details Pour éviter les modifications concurrentes du cache de styles
     * \~english \brief Mutual exclusion
     * \details To avoid concurrent styles cache updates
     */
    static std::mutex mtx;

public:


    /**
     * \~french \brief Vide l'annuaire et met le contenu à la corbeille
     * \~english \brief Empty book and put content into trash
     */
    static void send_to_trash ();

    /**
     * \~french \brief Vide la corbeille
     * \~english \brief Empty trash
     */
    static void empty_trash ();


    /**
     * \~french \brief Renseigne le répertoire des style
     * \~english \brief Set styles directory
     */
    static void set_directory (std::string d);

    /**
     * \~french \brief Retourne l'ensemble de l'annuaire
     * \~english \brief Return the book
     */
    static std::map<std::string,Style*> get_book ();

    /**
     * \~french
     * \brief Retourne le style d'après son identifiant
     * \details Si le style demandé n'est pas encore dans l'annuaire, ou que l'on ne veut pas de cache, il est recherché dans le répertoire connu et chargé
     * \param[in] id Identifiant du style voulu

     * \brief Return the style according to its identifier
     * \details If style is still not in the book, or cache is disabled, it is searched in the known directory and loaded
     * \param[in] id Wanted style identifier
     */
    static Style* get_style(std::string id);

    /**
     * \~french \brief Retourne le nombre de styles dans l'annuaire
     * \~english \brief Return the number of styles in the book
     */
    static int get_styles_count ();

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~StyleBook();

};