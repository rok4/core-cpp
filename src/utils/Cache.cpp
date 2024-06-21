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
std::mutex StyleBook::mtx;

Context * StoragePool::get_context(ContextType::eContextType type, std::string tray, Context* reference_context) {

    if (reference_context != 0 && reference_context->get_type() != type) {
        BOOST_LOG_TRIVIAL(error) << "Asked storage context and reference one have to own the same type";
        return NULL;
    }

    if (type == ContextType::S3CONTEXT) {
        // Dans le cas S3, le nom du bucket peut contenir le nom du cluster ou pas
        // Pour voir si on a déjà ce contexte de stockage, il faut que "tray" contienne le nom du bucket et du cluster
        // pour ne pas confondre 2 buckets avec le même nom sur deux clusters différents
        // On va donc s'assurer d'avoir ce nom de cluster :
        //    - soit il y est déjà et on le laisse
        //    - soit il n'y est pas et on met celui du contexte de référence
        //    - soit il n'y est pas et pas de contexte de référence => on met celui par défaut

        size_t pos = tray.find ( "@" );
        if ( pos == std::string::npos ) {
            std::string cluster_name;
            if (reference_context == 0) {
                cluster_name = S3Context::get_default_cluster();
                if (cluster_name == "") {
                    // Le chargement des informations a échoué (déjà loggé)
                    return NULL;
                }
            } else {
                // On ajoute le nom du cluster au nom du bucket
                cluster_name = ((S3Context *)reference_context)->getCluster();
            }

            tray = tray + "@" + cluster_name;
        }
    }

    Context* ctx;
    std::pair<ContextType::eContextType,std::string> key = make_pair(type,tray);

    std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = pool.find (key);
    if ( it != pool.end() ) {
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
                ctx = new S3Context(tray);
                break;
            case ContextType::FILECONTEXT:
                ctx = new FileContext(tray);
                break;
            default:
                BOOST_LOG_TRIVIAL(error) << "Unhandled storage context type";
                return NULL;
        }

        // on connecte pour vérifier que ce contexte est valide
        if (! ctx->connection()) {
            BOOST_LOG_TRIVIAL(error) << "Cannot connect " << ContextType::to_string(type) << " storage context, tray '" << tray << "'";
            delete ctx;
            return NULL;
        }

        BOOST_LOG_TRIVIAL(debug) << "Add storage context " << ContextType::to_string(type) << ", tray '" << tray << "'";
        pool.insert(make_pair(key,ctx));

        return ctx;
    }
}