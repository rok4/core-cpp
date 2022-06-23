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

#ifndef CACHE_H
#define CACHE_H

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
#include "storage/Context.h"
#include "storage/FileContext.h"
#if OBJECT_ENABLED
    #include "storage/object/SwiftContext.h"
    #include "storage/object/S3Context.h"
    #include "storage/object/CephPoolContext.h"
#endif

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un pool d'environnement Curl
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class CurlPool {  

private:

    /**
     * \~french \brief Annuaire des objet Curl
     * \details La clé est l'identifiant du thread
     * \~english \brief Curl object book
     * \details Key is the thread's ID
     */
    static std::map<pthread_t, CURL*> pool;

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    CurlPool(){};

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~CurlPool(){};

    /**
     * \~french \brief Retourne un objet Curl propre au thread appelant
     * \details Si il n'existe pas encore d'objet curl pour ce tread, on le crée et on l'initialise
     * \~english \brief Get the curl object specific to the calling thread
     * \details If curl object doesn't exist for this thread, it is created and initialized
     */
    static CURL* getCurlEnv() {
        pthread_t i = pthread_self();

        std::map<pthread_t, CURL*>::iterator it = pool.find ( i );
        if ( it == pool.end() ) {
            CURL* c = curl_easy_init();
            pool.insert ( std::pair<pthread_t, CURL*>(i,c) );
            return c;
        } else {
            curl_easy_reset(it->second);
            return it->second;
        }
    }

    /**
     * \~french \brief Affiche le nombre d'objet curl dans l'annuaire
     * \~english \brief Print the number of curl objects in the book
     */
    static void printNumCurls () {
        BOOST_LOG_TRIVIAL(info) << "Nombre de contextes curl : " << pool.size();
    }

    /**
     * \~french \brief Nettoie tous les objets curl dans l'annuaire et le vide
     * \~english \brief Clean all curl objects in the book and empty it
     */
    static void cleanCurlPool () {
        std::map<pthread_t, CURL*>::iterator it;
        for (it = pool.begin(); it != pool.end(); ++it) {
            curl_easy_cleanup(it->second);
        }
        pool.clear();
    }

};


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
    ProjPool(){};

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ProjPool(){};

    /**
     * \~french \brief Retourne un contexte Proj propre au thread appelant
     * \details Si il n'existe pas encore de contexte Proj pour ce tread, on le crée et on l'initialise
     * \~english \brief Get the Proj context specific to the calling thread
     * \details If Proj context doesn't exist for this thread, it is created and initialized
     */
    static PJ_CONTEXT* getProjEnv() {
        pthread_t i = pthread_self();

        std::map<pthread_t, PJ_CONTEXT*>::iterator it = pool.find ( i );
        if ( it == pool.end() ) {
            PJ_CONTEXT* pjc = proj_context_create();
            proj_log_level(pjc, PJ_LOG_NONE);
            pool.insert ( std::pair<pthread_t, PJ_CONTEXT*>(i,pjc) );
            return pjc;
        } else {
            return it->second;
        }
    }

    /**
     * \~french \brief Affiche le nombre de contextes proj dans l'annuaire
     * \~english \brief Print the number of proj contexts in the book
     */
    static void printNumProjs () {
        BOOST_LOG_TRIVIAL(info) <<  "Nombre de contextes proj : " << pool.size() ;
    }

    /**
     * \~french \brief Nettoie tous les contextes proj dans l'annuaire et le vide
     * \~english \brief Clean all proj objects in the book and empty it
     */
    static void cleanProjPool () {
        std::map<pthread_t, PJ_CONTEXT*>::iterator it;
        for (it = pool.begin(); it != pool.end(); ++it) {
            proj_context_destroy(it->second);
        }
        pool.clear();
    }

};


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un pool de contextes de stockage
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class StoragePool {

private:

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    StoragePool(){};

    /**
     * \~french \brief Annuaire de contextes
     * \details La clé est une paire composée du type de stockage et du contenant du contexte
     * \~english \brief Book of contexts
     * \details Key is a pair composed of type of storage and the context's bucket
     */
    static std::map<std::pair<ContextType::eContextType,std::string>,Context*> pool;


public:


    /**
     * \~french \brief Retourne une chaîne de caracère décrivant l'annuaire
     * \~english \brief Return a string describing the pool
     */
    static std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Context pool -------" << std::endl;
        oss << "\t- context number = " << pool.size() << std::endl;

        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.begin();
        while (it != pool.end()) {
            std::pair<ContextType::eContextType,std::string> key = it->first;
            oss << "\t\t- pot = " << key.first << "/" << key.second << std::endl;
            oss << it->second->toString() << std::endl;
            it++;
        }

        return oss.str() ;
    }

    /**
     * \~french
     * \brief Retourne le context correspondant au contenant demandé
     * \details Si il n'existe pas, une erreur s'affiche et on retourne NULL
     * \param[in] type Type de stockage du contexte rechercé
     * \param[in] tray Nom du contenant pour lequel on veut le contexte
     * \~english
     * \brief Return context of this tray
     * \details If context dosn't exist for this tray, an error is print and NULL is returned
     * \param[in] type storage type of looked for's context 
     * \param[in] tray Tray's name for which context is wanted
     */
    static Context* getContext(ContextType::eContextType type,std::string tray) {
        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.find (make_pair(type,tray));
        if ( it == pool.end() ) {
            BOOST_LOG_TRIVIAL(error) << "Le contenant demandé n'a pas été trouvé dans l'annuaire.";
            return NULL;
        } else {
            //le contenant est déjà existant et donc connecté
            return it->second;
        }
    }

    /**
     * \~french
     * \brief Ajoute un nouveau contexte
     * \details Si un contexte existe déjà pour ce nom de contenant, on ne crée pas de nouveau contexte et on retourne celui déjà existant. Le nouveau contexte n'est pas connecté.
     * \param[in] type type de stockage pour lequel on veut créer un contexte
     * \param[in] tray Nom du contenant pour lequel on veut créer un contexte
     * \param[in] ctx* contexte à ajouter

     * \brief Add a new context
     * \details If a context already exists for this tray's name, we don't create a new one and the existing is returned. New context is not connected.
     * \param[in] type Storage Type for which context is created
     * \param[in] tray Tray's name for which context is created
     * \param[in] ctx* Context to add
     
     */
    static Context * addContext(ContextType::eContextType type,std::string tray) {
        Context* ctx;
        std::pair<ContextType::eContextType,std::string> key = make_pair(type,tray);
        BOOST_LOG_TRIVIAL(debug) << "On essaye d'ajouter la clé " << ContextType::toString(key.first) <<" / " << key.second ;

        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.find (key);
        if ( it != pool.end() ) {
            //le contenant est déjà existant et donc connecté
            return it->second;

        } else {
            // ce contenant n'est pas encore connecté, on va créer la connexion
            // on créé le context selon le type de stockage
            switch(type){
#if OBJECT_ENABLED
                case ContextType::SWIFTCONTEXT:
                    ctx = new SwiftContext(tray);
                    break;
                case ContextType::CEPHCONTEXT:
                    ctx = new CephPoolContext(tray);
                    break;
                case ContextType::S3CONTEXT:
                    ctx = new S3Context(tray);
                    break;
#endif
                case ContextType::FILECONTEXT:
                    ctx = new FileContext(tray);
                    break;
                default:
                    //ERREUR
                    BOOST_LOG_TRIVIAL(error) << "Ce type de contexte n'est pas géré.";
                    return NULL;
            }

            // on connecte pour vérifier que ce contexte est valide
            if (!(ctx->connection())) {
                BOOST_LOG_TRIVIAL(error) << "Impossible de connecter au contexte de type " << ContextType::toString(type) << ", contenant " << tray;
                delete ctx;
                return NULL;
            }


            //BOOST_LOG_TRIVIAL(debug) << "On insère ce contexte " << ctx->toString() ;
            pool.insert(make_pair(key,ctx));

            return ctx;
        }

    }


    /**
     * \~french \brief Affiche le nombre de contextes de stockage dans l'annuaire
     * \~english \brief Print the number of storage contexts in the book
     */
    static void printNumStorages () {
        BOOST_LOG_TRIVIAL(info) <<  "Nombre de contextes de stockage : " << pool.size() ;
    }

    /**
     * \~french \brief Obtient l'annuaire de contextes
     * \details La clé est une paire composée du type de stockage et du contenant du contexte
     * \~english \brief Get book of contexts
     * \details Key is a pair composed of type of storage and the context's bucket
     */
    static std::map<std::pair<ContextType::eContextType,std::string>,Context*> getPool() {
        return pool;
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~StoragePool() {};

    /**
     * \~french \brief Nettoie tous les contextes de stockage dans l'annuaire et le vide
     * \~english \brief Clean all storage context objects in the book and empty it
     */
    static void cleanStoragePool () {
        std::map<std::pair<ContextType::eContextType,std::string>,Context*>::iterator it;
        for (it=pool.begin(); it!=pool.end(); ++it) {
            delete it->second;
            it->second = NULL;
        }
    }

};


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Élément du cache des index de dalle
 */
class CacheElement {
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
    CacheElement(std::string k, Context* c, std::string n, int tiles_number, uint8_t* os, uint8_t* ss) {
        key = k;
        context = c;
        name = n;
        date = std::time(NULL);
        for (int i = 0; i < tiles_number; i++) {
            offsets.push_back(*((uint32_t*) (os + i*4)));
            sizes.push_back(*((uint32_t*) (ss + i*4)));
        }        
    };
};

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
    static std::list<CacheElement *> cache;
    /**
     * \~french \brief Map d'index des éléments du cache
     * \~english \brief Cache element index map
     */
    static std::unordered_map<std::string, std::list<CacheElement *>::iterator> map;
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
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    IndexCache(){};

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~IndexCache(){};

    /** \~french
     * \brief Définit la taille du cache
     * \param[in] s taille du cache
     ** \~english
     * \brief Define cache size
     * \param[in] s cache size
     */
    static void setCacheSize(int s) {
        size = s;
    };

    /** \~french
     * \brief Définit la durée de validité
     * \param[in] s durée de validité du cache, en secondes
     ** \~english
     * \brief Define cache validity
     * \param[in] s cache validity, in seconds
     */
    static void setValidity(int v) {
        validity = v;
    };

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
    static void add_slab_infos(std::string origin_slab_name, Context* data_context, std::string data_slab_name, int tiles_number, uint8_t* offsets, uint8_t* sizes) {
        // On utilise le nom original de la dalle (celle à lire a priori) comme clé dans la map
        // Potentiellement ce nom est différent de la vraie dalle contenant la donnée (dans le cas d'une dalle symbolique)
        // mais c'est via cette dalle symbolique que la donnée est a priori requêtée
        // donc c'est ce nom qu'on utilisera pour l'interrogation du cache

        CacheElement* elem = new CacheElement(origin_slab_name, data_context, data_slab_name, tiles_number, offsets, sizes);

        // L'information n'est a priori pas dans le cache car 
        // Soit elle était dans le cache et valide, alors on aurait utiliser ce cahe et on n'aurait pas fait appel à cet ajout
        // Soit elle était dans le cache mais obsolète, alors on l'aurait déjà supprimé du cache

        if (cache.size() == size) {
            // On réupcère le dernier élément du cache
            CacheElement* last = cache.back();
            // On le vire du cache
            cache.pop_back();
            // On le déréférence de la map d'accès
            map.erase(last->key);
            delete last;
        }
    
        // update reference
        cache.push_front(elem);
        map[origin_slab_name] = cache.begin();
    };

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
    static bool get_slab_infos(std::string key, int tile_number, Context** data_context, std::string* data_slab_name, uint32_t* offset, uint32_t* size) {
        std::unordered_map<std::string, std::list<CacheElement *>::iterator>::iterator it = map.find ( key );
        if ( it == map.end() ) {
            return false;
        } else {
            // Gestion de la péremption du cache (une heure max)
            std::time_t now = std::time(NULL);
            if (now - (*(it->second))->date > validity) {
                delete *(it->second);
                cache.erase(it->second);
                map.erase(it);
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
    };

    /**
     * \~french \brief Nettoie tous les objets dans le cache
     * \~english \brief Clean all element from the cache
     */
    static void cleanCache () {
        std::list<CacheElement*>::iterator it;
        for (it = cache.begin(); it != cache.end(); ++it) {
            delete *it;
        }
        cache.clear();
    }
};

#endif
