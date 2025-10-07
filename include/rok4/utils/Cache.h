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
 * \file Cache.h
 ** \~french
 * \brief Définition des classes IndexCache, CurlPool, StoragePool et ProjPool
 ** \~english
 * \brief Define classes IndexCache, CurlPool, StoragePool and ProjPool
 */

#pragma once

#include <stdint.h>// pour uint8_t
#include <boost/log/trivial.hpp>
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <sstream>
#include <curl/curl.h>
#include <proj.h>
#include <thread>
#include <mutex>


#include "rok4/utils/TileMatrixSet.h"
#include "rok4/style/Style.h"
#include "rok4/utils/Utils.h"
#include "rok4/utils/CRS.h"
#include "rok4/utils/TmsBook.h"
#include "rok4/utils/IndexCache.h"
#include "rok4/storage/Context.h"


#define ROK4_TMS_DIRECTORY "ROK4_TMS_DIRECTORY"
#define ROK4_TMS_NO_CACHE "ROK4_TMS_NO_CACHE"
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
    StyleBook(){};

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
    static void send_to_trash () {
        mtx.lock();
        std::map<std::string, Style*>::iterator it;
        for (it = book.begin(); it != book.end(); ++it) {
            trash.push_back(it->second);
        }
        book.clear();
        mtx.unlock();
    }

    /**
     * \~french \brief Vide la corbeille
     * \~english \brief Empty trash
     */
    static void empty_trash () {
        mtx.lock();
        for (int i = 0; i < trash.size(); i++) {
            delete trash.at(i);
        }
        trash.clear();
        mtx.unlock();
    }


    /**
     * \~french \brief Renseigne le répertoire des style
     * \~english \brief Set styles directory
     */
    static void set_directory (std::string d) {
        // Suppression du slash final
        if (d.compare ( d.size()-1,1,"/" ) == 0) {
            d.pop_back();
        }
        directory = d;
    }

    /**
     * \~french \brief Retourne l'ensemble de l'annuaire
     * \~english \brief Return the book
     */
    static std::map<std::string,Style*> get_book () {
        return book;
    }

    /**
     * \~french
     * \brief Retourne le style d'après son identifiant
     * \details Si le style demandé n'est pas encore dans l'annuaire, ou que l'on ne veut pas de cache, il est recherché dans le répertoire connu et chargé
     * \param[in] id Identifiant du style voulu

     * \brief Return the style according to its identifier
     * \details If style is still not in the book, or cache is disabled, it is searched in the known directory and loaded
     * \param[in] id Wanted style identifier
     */
    static Style* get_style(std::string id) {

        std::map<std::string, Style*>::iterator it = book.find ( id );
        if ( it != book.end() ) {
            return it->second;
        }

        std::string d;
        if (directory.empty()) {
            char* e = getenv (ROK4_STYLES_DIRECTORY);
            if (e == NULL) {
                d.assign("/usr/share/rok4/styles");
            } else {
                d.assign(e);
            }
        } else {
            d = directory;
        }

        std::string style_path = d + "/" + id;

        mtx.lock();
        // Si on fait du cache de style, on revérifie que ce style n'a pas déjà été ajouté par un thread concurrent entre temps
        if(getenv (ROK4_STYLES_NO_CACHE) == NULL) {
            if ( it != book.end() ) {
                // On a effectivement depuis déjà enregistré ce style
                return it->second;
            }
        }

        Style* style = new Style(style_path);
        if ( ! style->is_ok() ) {
            BOOST_LOG_TRIVIAL(error) << style->get_error_message();
            delete style;
            mtx.unlock();
            return NULL;
        }

        if ( contain_chars(style->get_identifier(), "<>") ) {
            BOOST_LOG_TRIVIAL(error) << "Style identifier contains forbidden chars" ;
            delete style;
            mtx.unlock();
            return NULL;
        }

        if(getenv (ROK4_STYLES_NO_CACHE) == NULL) {
            // On veut utiliser le cache, on met donc ce nouveau style dans l'annuaire pour le trouver la porchaine fois
            book.insert ( std::pair<std::string, Style*>(id, style) );
        } else {
            // On met le style directement dans la corbeille, pour que le nettoyage se fasse bien
            trash.push_back(style);
        }

        mtx.unlock();
        return style;
    }

    /**
     * \~french \brief Retourne le nombre de styles dans l'annuaire
     * \~english \brief Return the number of styles in the book
     */
    static int get_styles_count () {
        return book.size();
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~StyleBook() {};

};


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
    CrsBook(){};

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
    static CRS* get_crs(std::string id) {

        id = to_upper_case(id);

        std::map<std::string, CRS*>::iterator it = book.find ( id );
        if ( it != book.end() ) {
            return it->second;
        }

        mtx.lock();

        CRS* crs = new CRS(id);
        // Le CRS est potentiellement non défini (si il n'est pas valide), on le mémorise pour ne pas réessayer la prochaine fois
        book.emplace(id, crs);

        mtx.unlock();
        return crs;
    }

    /**
     * \~french \brief Nettoie tous les CRS dans l'annuaire et le vide
     * \~english \brief Clean all CRS objects in the book and empty it
     */
    static void clean_crss () {
        mtx.lock();
        std::map<std::string, CRS*>::iterator it;
        for (it = book.begin(); it != book.end(); ++it) {
            delete it->second;
        }
        book.clear();
        mtx.unlock();
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~CrsBook() {};

};


