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
 * \file CephPoolContext.cpp
 ** \~french
 * \brief Implémentation de la classe CephPoolContext
 * \details
 * \li CephPoolContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Implement classe CephPoolContext
 * \details
 * \li CephPoolContext : Ceph data pool connection
 */

#include "storage/ceph/CephPoolContext.h"
#include <stdlib.h>


CephPoolContext::CephPoolContext (std::string pool) : Context(), pool_name(pool) {

    char* cluster = getenv (ROK4_CEPH_CLUSTERNAME);
    if (cluster == NULL) {
        cluster_name.assign("ceph");
    } else {
        cluster_name.assign(cluster);
    }

    char* user = getenv (ROK4_CEPH_USERNAME);
    if (user == NULL) {
        user_name.assign("client.admin");
    } else {
        user_name.assign(user);
    }

    char* conf = getenv (ROK4_CEPH_CONFFILE);
    if (conf == NULL) {
        conf_file.assign("/etc/ceph/ceph.conf");
    } else {
        conf_file.assign(conf);
    }
}

bool CephPoolContext::connection() {

    if (! connected) {
        uint64_t flags;
        int ret = 0;

        ret = rados_create2(&cluster, cluster_name.c_str(), user_name.c_str(), flags);
        if (ret < 0) {
            BOOST_LOG_TRIVIAL(error) << "Couldn't initialize the cluster handle! error " << ret;
            return false;
        }

        ret = rados_conf_read_file(cluster, conf_file.c_str());
        if (ret < 0) {
            BOOST_LOG_TRIVIAL(error) <<  "Couldn't read the Ceph configuration file! error " << ret ;
            BOOST_LOG_TRIVIAL(error) << strerror(-ret);
            BOOST_LOG_TRIVIAL(error) <<  "Configuration file : " << conf_file ;
            return false;
        }

        // On met les timeout à 10 minutes
        rados_conf_set(cluster, "client_mount_timeout", "60");
        rados_conf_set(cluster, "rados_mon_op_timeout", "60");
        rados_conf_set(cluster, "rados_osd_op_timeout", "60");

        ret = rados_connect(cluster);
        if (ret < 0) {
            BOOST_LOG_TRIVIAL(error) <<  "Couldn't connect to cluster! error " << ret ;
            BOOST_LOG_TRIVIAL(error) << strerror(-ret);
            return false;
        }

        ret = rados_ioctx_create(cluster, pool_name.c_str(), &io_ctx);
        if (ret < 0) {
            BOOST_LOG_TRIVIAL(error) <<  "Couldn't set up ioctx! error " << ret ;
            BOOST_LOG_TRIVIAL(error) << strerror(-ret);
            BOOST_LOG_TRIVIAL(error) <<  "Pool : " << pool_name ;
            rados_shutdown(cluster);
            return false;
        }

        connected = true;
    }

    return true;
}

int CephPoolContext::read(uint8_t* data, int offset, int size, std::string name) {
   
    BOOST_LOG_TRIVIAL(debug) << "Ceph read : " << size << " bytes (from the " << offset << " one) in the object " << pool_name << " / " << name;

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Try to read using the unconnected ceph pool context " << pool_name;
        return -1;
    }

    int readSize;
    int attempt = 1;
    bool error = false;
    while(attempt <= read_attempts) {
        readSize = rados_read(io_ctx, name.c_str(), (char*) data, size, offset);

        if (readSize < 0) {
            error = true;
            // Seul le timeout donne lieu à une nouvelle tentative
            if (readSize == -ETIMEDOUT) {
                BOOST_LOG_TRIVIAL(warning) <<  "Try " << attempt << " timed out" ;
            } else {
                BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
                BOOST_LOG_TRIVIAL(error) << "Error code: " << readSize ;
                BOOST_LOG_TRIVIAL(error) << strerror(-readSize);
                break;
            }
        } else {
            error = false;
            break;
        }

        attempt++;
        sleep(waiting_time);
    }

    if (error) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read " << size << " bytes (from the " << offset << " one) in the Ceph object " << pool_name << " / " << name  << " after " << read_attempts << " tries" ;
    }

    return readSize;
}


uint8_t* CephPoolContext::read_full(int& size, std::string name) {

    size = -1;
    
    BOOST_LOG_TRIVIAL(debug) << "Ceph read full : " << pool_name << " / " << name;

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Try to read using the unconnected ceph pool context " << pool_name;
        return NULL;
    }

    // Récupération de la taille
    uint64_t fullSize;
    time_t time;
    int ret = rados_stat(io_ctx, name.c_str(), &fullSize, &time);
    if (ret < 0) {
        BOOST_LOG_TRIVIAL(error) << "Error reading size of ceph object " << name;
        BOOST_LOG_TRIVIAL(error) << strerror(-ret);
        return NULL;
    }

    uint8_t* data = new uint8_t(fullSize);

    int attempt = 1;
    bool error = false;
    while(attempt <= read_attempts) {
        size = rados_read(io_ctx, name.c_str(), (char*) data, fullSize, 0);

        if (size < 0) {
            error = true;
            // Seul le timeout donne lieu à une nouvelle tentative
            if (size == -ETIMEDOUT) {
                BOOST_LOG_TRIVIAL(warning) <<  "Try " << attempt << " timed out" ;
            } else {
                BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
                BOOST_LOG_TRIVIAL(error) << "Error code: " << size ;
                BOOST_LOG_TRIVIAL(error) << strerror(-size);
                break;
            }
        } else {
            error = false;
            break;
        }

        attempt++;
        sleep(waiting_time);
    }

    if (error) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read full Ceph object " << pool_name << " / " << name  << " after " << read_attempts << " tries" ;
    }

    return data;
}


bool CephPoolContext::write(uint8_t* data, int offset, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Ceph write : " << size << " bytes (from the " << offset << " one) in the writing buffer " << name;

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
        // pas de buffer pour ce nom d'objet
        BOOST_LOG_TRIVIAL(error) << "No writing buffer for the name " << name;
        return false;
    }
   
    // Calcul de la taille finale et redimensionnement éventuel du vector
    if (it1->second->size() < size + offset) {
        it1->second->resize(size + offset);
    }

    memcpy(&((*(it1->second))[0]) + offset, data, size);

    return true;
}

bool CephPoolContext::write_full(uint8_t* data, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Ceph write : " << size << " bytes (one shot) in the writing buffer " << name;

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
        // pas de buffer pour ce nom d'objet
        BOOST_LOG_TRIVIAL(error) << "No Ceph writing buffer for the name " << name;
        return false;
    }

    it1->second->clear();

    it1->second->resize(size);
    memcpy(&((*(it1->second))[0]), data, size);

    return true;
}

ContextType::eContextType CephPoolContext::get_type() {
    return ContextType::CEPHCONTEXT;
}

std::string CephPoolContext::get_type_string() {
    return "CEPHCONTEXT";
}

std::string CephPoolContext::get_tray() {
    return pool_name;
}

bool CephPoolContext::open_to_write(std::string name) {

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 != write_buffers.end() ) {
        BOOST_LOG_TRIVIAL(error) << "A Ceph writing buffer already exists for the name " << name;
        return false;

    } else {
        write_buffers.insert ( std::pair<std::string,std::vector<char>*>(name, new std::vector<char>()) );
    }

    return true;
}


bool CephPoolContext::close_to_write(std::string name) {


    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
        BOOST_LOG_TRIVIAL(error) << "The Ceph writing buffer with name " << name << "does not exist, cannot flush it";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "Write buffered " << it1->second->size() << " bytes in the ceph object " << name;

    bool ok = true;
    int attempt = 1;
    while(attempt <= write_attempts) {
        int err = rados_write_full(io_ctx,name.c_str(), &((*(it1->second))[0]), it1->second->size());
        if (err < 0) {
            ok = false;
            BOOST_LOG_TRIVIAL(warning) <<  "Try " << attempt ;
            BOOST_LOG_TRIVIAL(warning) << "Error code: " << err ;
            BOOST_LOG_TRIVIAL(warning) << strerror(-err);
        } else {
            ok = true;
            break;
        }

        attempt++;
        sleep(waiting_time);
    }

    if (ok) {
        BOOST_LOG_TRIVIAL(debug) << "Erase the flushed buffer";
        delete it1->second;
        write_buffers.erase(it1);
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to flush " << it1->second->size() << " bytes in the object " << name << " after " << write_attempts << " tries" ;
    }
    return ok;
}

std::string CephPoolContext::get_path(std::string racine,int x,int y,int pathDepth){
    return racine + "_" + std::to_string(x) + "_" + std::to_string(y);
}

std::string CephPoolContext::get_path(std::string name) {  
    return pool_name + "/" + name;
}

bool CephPoolContext::exists(std::string name) {

    BOOST_LOG_TRIVIAL(debug) << "Exists (CEPH) ? " << get_path(name);

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Try to test object existence using the unconnected ceph pool context " << pool_name;
        return false;
    }

    uint64_t fullSize;
    time_t time;
    int ret = rados_stat(io_ctx, name.c_str(), &fullSize, &time);
    if (ret < 0) {
        return false;
    } else {
        return true;
    }
}