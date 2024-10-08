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
 * \file CephPoolContext.h
 ** \~french
 * \brief Définition de la classe CephPoolContext
 * \details
 * \li CephPoolContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Define classe CephPoolContext
 * \details
 * \li CephPoolContext : Ceph data pool connection
 */

#pragma once

#include <rados/librados.h>
#include <boost/log/trivial.hpp>
#include "storage/Context.h"

#define ROK4_CEPH_USERNAME "ROK4_CEPH_USERNAME"
#define ROK4_CEPH_CLUSTERNAME "ROK4_CEPH_CLUSTERNAME"
#define ROK4_CEPH_CONFFILE "ROK4_CEPH_CONFFILE"


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte Ceph (connexion à un cluster + pool particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class CephPoolContext : public Context {
    
private:
    
    /**
     * \~french \brief Nom du cluster ceph
     * \~english \brief Name of ceph cluster
     */
    std::string cluster_name;
    
    /**
     * \~french \brief Utilisateur du cluster ceph
     * \~english \brief User of ceph cluster
     */
    std::string user_name;
    
    /**
     * \~french \brief Fichier de configuration du cluster ceph
     * \~english \brief Configuration file of ceph cluster
     */
    std::string conf_file;
    
    /**
     * \~french \brief Nom du pool ceph
     * \~english \brief Name of ceph pool
     */
    std::string pool_name;

    /**
     * \~french \brief Objet "cluster" librados, pour gérer le cluster
     * \~english \brief "cluster" librados object, to handle the cluster
     */
    rados_t cluster;
    /**
     * \~french \brief Objet "contexte" librados, pour communiquer avec un pool précis
     * \~english \brief "context" librados object, to communicate with a specific pool
     */
    rados_ioctx_t io_ctx;

public:

    /**
     * \~french
     * \brief Constructeur pour un contexte Ceph, avec les valeur par défaut
     * \details Les valeurs sont récupérées dans les variables d'environnement ou sont celles par défaut
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH><TH>Valeur par défaut</TH>
     * <TR><TD>user_name</TD><TD>ROK4_CEPH_USERNAME</TD><TD>client.admin</TD>
     * <TR><TD>cluster_name</TD><TD>ROK4_CEPH_CLUSTERNAME</TD><TD>ceph</TD>
     * <TR><TD>conf_file</TD><TD>ROK4_CEPH_CONFFILE</TD><TD>/etc/ceph/ceph.conf</TD>
     * </TABLE>
     * \param[in] pool Pool avec lequel on veut communiquer
     * \~english
     * \brief Constructor for Ceph context, with default value
     * \details Values are read in environment variables, or are deulat one
     * <TABLE>
     * <TR><TH>Attribute</TH><TH>Environment variables</TH><TH>Default value</TH>
     * <TR><TD>user_name</TD><TD>ROK4_CEPH_USERNAME</TD><TD>client.admin</TD>
     * <TR><TD>cluster_name</TD><TD>ROK4_CEPH_CLUSTERNAME</TD><TD>ceph</TD>
     * <TR><TD>conf_file</TD><TD>ROK4_CEPH_CONFFILE</TD><TD>/etc/ceph/ceph.conf</TD>
     * </TABLE>
     * \param[in] pool Pool to use
     */
    CephPoolContext (std::string pool);

    ContextType::eContextType get_type();
    std::string get_type_string();
    std::string get_tray();
    
    /**
     * \~french \brief Instancie les objets librados #cluster and #io_ctx
     * \~english \brief Instanciate librados objects #cluster and #io_ctx
     */
    bool connection();

    bool exists(std::string name);

    /**
     * \~french \brief Nettoie les objets librados
     * \~english \brief Clean librados objects
     */
    void close_connection() {
        if (connected) {
            rados_aio_flush(io_ctx);
            rados_ioctx_destroy(io_ctx);
            rados_shutdown(cluster);
            connected = false;
        }
    }

    /**
     * \~french \brief Retourne le nom de l'utilisateur ceph
     * \~english \brief Return the name of ceph user
     */
    std::string getPoolUser () {
        return user_name;
    }

    /**
     * \~french \brief Retourne le fichier de configuration ceph
     * \~english \brief Return the ceph configuration file
     */
    std::string getPoolConf () {
        return conf_file;
    }

    /**
     * \~french \brief Retourne le nom du cluster ceph
     * \~english \brief Return the name of ceph cluster
     */
    std::string getClusterName () {
        return cluster_name;
    }

    int read(uint8_t* data, int offset, int size, std::string name);
    uint8_t* read_full(int& size, std::string name);
    bool write(uint8_t* data, int offset, int size, std::string name);
    bool write_full(uint8_t* data, int size, std::string name);

    bool open_to_write(std::string name);
    bool close_to_write(std::string name);
    
    std::string get_path(std::string racine,int x,int y,int pathDepth);
    std::string get_path(std::string name);

    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "------ Ceph Context -------" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- cluster name = " << cluster_name ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- user name = " << user_name ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- configuration file = " << conf_file ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- pool name = " << pool_name ;
    }

    std::string to_string() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Ceph Context -------" << std::endl;
        oss << "\t- cluster name = " << cluster_name << std::endl;
        oss << "\t- user name = " << user_name << std::endl;
        oss << "\t- configuration file = " << conf_file << std::endl;
        oss << "\t- pool name = " << pool_name << std::endl;
        if (connected) {
            oss << "\t- CONNECTED !" << std::endl;
        } else {
            oss << "\t- NOT CONNECTED !" << std::endl;
        }
        return oss.str() ;
    }
    
    ~CephPoolContext() {
        close_connection();
    }
};


