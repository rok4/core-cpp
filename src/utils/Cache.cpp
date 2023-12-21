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
 * \file Cache.cpp
 ** \~french
 * \brief Implémentation des classes IndexCache, CurlPool, StoragePool et ProjPool
 ** \~english
 * \brief Implements classes IndexCache, CurlPool, StoragePool and ProjPool
 */

#include "utils/Cache.h"
#include "storage/FileContext.h"
#include "storage/SwiftContext.h"
#include "storage/S3Context.h"
#if CEPH_ENABLED
    #include "storage/ceph/CephPoolContext.h"
#endif

std::map<pthread_t, CURL*> CurlPool::pool;

std::map<pthread_t, PJ_CONTEXT*> ProjPool::pool;

std::map<std::pair<ContextType::eContextType,std::string>,Context*> StoragePool::pool;

std::list<IndexElement *> IndexCache::cache;
std::unordered_map<std::string, std::list<IndexElement *>::iterator> IndexCache::map;
int IndexCache::size = 100;
int IndexCache::validity = 300;
std::mutex IndexCache::mtx;

std::map<std::string, TileMatrixSet*> TmsBook::book;
std::vector<TileMatrixSet*> TmsBook::trash;
std::string TmsBook::directory = "";
std::mutex TmsBook::mtx;

std::map<std::string, Style*> StyleBook::book;
std::vector<Style*> StyleBook::trash;
std::string StyleBook::directory = "";
bool StyleBook::inspire = false;
std::mutex StyleBook::mtx;

Context * StoragePool::get_context(ContextType::eContextType type,std::string tray) {
    Context* ctx;
    std::pair<ContextType::eContextType,std::string> key = make_pair(type,tray);

    std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.find (key);
    if ( it != pool.end() ) {
        BOOST_LOG_TRIVIAL(debug) << "Storage context already added " << ContextType::toString(it->first.first) << " / '" << it->first.second << "'" ;
        // le contenant est déjà existant et donc connecté
        return it->second;

    } else {
        // ce contenant n'est pas encore connecté, on va créer la connexion
        // on créé le context selon le type de stockage
        switch(type){
#if CEPH_ENABLED
            case ContextType::CEPHCONTEXT:
                ctx = new CephPoolContext(tray);
                break;
#endif
            case ContextType::SWIFTCONTEXT:
                ctx = new SwiftContext(tray);
                break;
            case ContextType::S3CONTEXT:
                // Pour S3, tray = bucket@cluster
                ctx = new S3Context(tray);
                if (! ((S3Context *) ctx)->isInitialized()) {
                    //ERREUR
                    BOOST_LOG_TRIVIAL(error) << "Le contexte n'a pas pu être initialisé !?";
                    return NULL;
                }
                break;
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

        BOOST_LOG_TRIVIAL(debug) << "Add storage context " << ContextType::toString(key.first) << " / '" << key.second << "'" ;
        pool.insert(make_pair(key,ctx));

        return ctx;
    }
}