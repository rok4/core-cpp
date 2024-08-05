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
#include <thread>
#include <mutex>


#include "rok4/utils/TileMatrixSet.h"
#include "rok4/style/Style.h"
#include "rok4/utils/Utils.h"
#include "rok4/storage/Context.h"

#define ROK4_TMS_DIRECTORY "ROK4_TMS_DIRECTORY"
#define ROK4_TMS_NO_CACHE "ROK4_TMS_NO_CACHE"
#define ROK4_STYLES_DIRECTORY "ROK4_STYLES_DIRECTORY"
#define ROK4_STYLES_NO_CACHE "ROK4_STYLES_NO_CACHE"

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
    static std::string to_string() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Context pool -------" << std::endl;
        oss << "\t- context number = " << pool.size() << std::endl;

        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.begin();
        while (it != pool.end()) {
            std::pair<ContextType::eContextType,std::string> key = it->first;
            oss << "\t\t- pot = " << key.first << "/" << key.second << std::endl;
            oss << it->second->to_string() << std::endl;
            it++;
        }

        return oss.str() ;
    }

    /**
     * \~french
     * \brief Récupère un contexte de stockage
     * \details Si un contexte existe déjà pour ce nom de contenant, on ne crée pas de nouveau contexte et on retourne celui déjà existant. Le nouveau contexte n'est pas connecté.
     * \param[in] type type de stockage pour lequel on veut créer un contexte
     * \param[in] tray Nom du contenant pour lequel on veut créer un contexte
     * \param[in] reference_context Contexte de stockage de référence

     * \brief Get a context storage
     * \details If a context already exists for this tray's name, we don't create a new one and the existing is returned. New context is not connected.
     * \param[in] type Storage Type for which context is created
     * \param[in] tray Tray's name for which context is created
     * \param[in] reference_context Reference storage context
     
     */
    static Context * get_context(ContextType::eContextType type,std::string tray, Context* reference_context = 0) ;


    /**
     * \~french \brief Retourne le nombre de contextes de stockage dans l'annuaire par type
     * \param[out] file_count Nombre de contexte de stockage fichier
     * \param[out] s3_count Nombre de contexte de stockage S3
     * \param[out] ceph_count Nombre de contexte de stockage Ceph
     * \param[out] swift_count Nombre de contexte de stockage Swift
     * \~english \brief Return the number of storage contexts in the book per type
     * \param[out] file_count File storage context count
     * \param[out] s3_count S3 storage context count
     * \param[out] ceph_count Ceph storage context count
     * \param[out] swift_count Swift storage context count
     */
    static void getStorageCounts (int& file_count, int& s3_count, int& ceph_count, int& swift_count) {
        file_count = 0;
        s3_count = 0;
        ceph_count = 0;
        swift_count = 0;
        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.begin();
        while (it != pool.end()) {
            std::pair<ContextType::eContextType,std::string> key = it->first;
            switch(key.first) {
#if CEPH_ENABLED
                case ContextType::CEPHCONTEXT:
                    ceph_count++;
                    break;
#endif
                case ContextType::SWIFTCONTEXT:
                    swift_count++;
                    break;
                case ContextType::S3CONTEXT:
                    s3_count++;
                    break;
                case ContextType::FILECONTEXT:
                    file_count++;
                    break;
            }
            it++;
        }
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
    IndexElement(std::string k, Context* c, std::string n, int tiles_number, uint8_t* os, uint8_t* ss) {
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

        mtx.lock();

        IndexElement* elem = new IndexElement(origin_slab_name, data_context, data_slab_name, tiles_number, offsets, sizes);

        // L'information n'est a priori pas dans le cache car 
        // Soit elle était dans le cache et valide, alors on aurait utilisé ce cache et on n'aurait pas fait appel à cet ajout
        // Soit elle était dans le cache mais obsolète, alors on l'aurait déjà supprimé du cache

        if (cache.size() == size) {
            // On récupère le dernier élément du cache
            IndexElement* last = cache.back();
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
        std::unordered_map<std::string, std::list<IndexElement *>::iterator>::iterator it = map.find ( key );
        if ( it == map.end() ) {
            return false;
        } else {
            // Gestion de la péremption du cache (une heure max)
            std::time_t now = std::time(NULL);
            if (now - (*(it->second))->date > validity) {
                mtx.lock();

                // on le cherche à nouveau pour vérifier qu'il n'a pas déjà été supprimé par un thread concurrent
                it = map.find ( key );

                if ( it != map.end() ) {
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
    };

    /**
     * \~french \brief Nettoie tous les objets dans le cache
     * \~english \brief Clean all element from the cache
     */
    static void cleanCache () {
        mtx.lock();
        std::list<IndexElement*>::iterator it;
        for (it = cache.begin(); it != cache.end(); ++it) {
            delete *it;
        }
        cache.clear();
        mtx.unlock();
    }
};


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un annuaire de Tile Matrix Sets
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class TmsBook {

private:

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    TmsBook(){};

    /**
     * \~french
     * \brief Répertoire de stockage des TMS
     * \~english
     * \brief TMS storage directory
     */
    static std::string directory;

    /**
     * \~french \brief Annuaire de TMS
     * \details La clé est l'identifiant du TMS
     * \~english \brief Book of TMS
     * \details Key is a the TMS identifier
     */
    static std::map<std::string,TileMatrixSet*> book;

    /**
     * \~french \brief Corbeille de TMS à supprimer
     * \~english \brief TMS trash to delete
     */
    static std::vector<TileMatrixSet*> trash;

    /**
     * \~french \brief Exclusion mutuelle
     * \details Pour éviter les modifications concurrentes du cache de TMS
     * \~english \brief Mutual exclusion
     * \details To avoid concurrent TMS cache updates
     */
    static std::mutex mtx;

public:


    /**
     * \~french \brief Vide l'annuaire et met le contenu à la corbeille
     * \~english \brief Empty book and put content into trash
     */
    static void send_to_trash () {
        mtx.lock();
        std::map<std::string, TileMatrixSet*>::iterator it;
        for (it = book.begin(); it != book.end(); ++it) {
            trash.push_back(it->second);
        }
        book.empty();
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
        trash.empty();
        mtx.unlock();
    }


    /**
     * \~french \brief Renseigne le répertoire des TMS
     * \~english \brief Set TMS directory
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
    static std::map<std::string,TileMatrixSet*> get_book () {
        return book;
    }

    /**
     * \~french
     * \brief Retourne le TMS d'après son identifiant
     * \details Si le TMS demandé n'est pas encore dans l'annuaire, ou que l'on ne veut pas de cache, il est recherché dans le répertoire connu et chargé
     * \param[in] id Identifiant du TMS voulu

     * \brief Retourne the TMS according to its identifier
     * \details If TMS is still not in the book, or cache is disabled, it is searched in the known directory and loaded
     * \param[in] id Wanted TMS identifier
     */
    static TileMatrixSet* get_tms(std::string id) {
        std::map<std::string, TileMatrixSet*>::iterator it = book.find ( id );
        if ( it != book.end() ) {
            return it->second;
        }

        std::string d;
        if (directory.empty()) {
            char* e = getenv (ROK4_TMS_DIRECTORY);
            if (e == NULL) {
                d.assign("/usr/share/rok4/tilematrixsets");
            } else {
                d.assign(e);
            }
        } else {
            d = directory;
        }

        std::string tms_path = d + "/" + id;

        mtx.lock();
        // Si on fait du cache de TMS, on revérifie que ce TMS n'a pas déjà été ajouté par un thread concurrent entre temps
        if(getenv (ROK4_TMS_NO_CACHE) == NULL) {
            if ( it != book.end() ) {
                // On a effectivement depuis déjà enregistré ce TMS
                return it->second;
            }
        }

        TileMatrixSet* tms = new TileMatrixSet(tms_path);
        if ( ! tms->isOk() ) {
            BOOST_LOG_TRIVIAL(error) << tms->getErrorMessage();
            delete tms;
            mtx.unlock();
            return NULL;
        }

        if(getenv (ROK4_TMS_NO_CACHE) == NULL) {
            // On veut utiliser le cache, on met donc ce nouveau TMS dans l'annuaire pour le trouver la prochaine fois
            book.insert ( std::pair<std::string, TileMatrixSet*>(id, tms) );
        } else {
            // On met le TMS directement dans la corbeille, pour que le nettoyage se fasse bien
            trash.push_back(tms);
        }
        
        mtx.unlock();

        return tms;
    }

    /**
     * \~french \brief Retourne le nombre de TMS dans l'annuaire
     * \~english \brief Return the number of TMS in the book
     */
    static int get_tms_count () {
        return book.size();
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~TmsBook() {};

};


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
     * \details Pour éviter les modifications concurrentes du cache de TMS
     * \~english \brief Mutual exclusion
     * \details To avoid concurrent TMS cache updates
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
        book.empty();
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
        trash.empty();
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

     * \brief Retourne the style according to its identifier
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
        if ( ! style->isOk() ) {
            BOOST_LOG_TRIVIAL(error) << style->getErrorMessage();
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

#endif
