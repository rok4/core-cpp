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
 * \file FileContext.h
 ** \~french
 * \brief Définition de la classe FileContext
 * \details
 * \li FileContext : utilisation d'un système de fichier
 ** \~english
 * \brief Define classe FileContext
 * \details
 * \li FileContext : file system use
 */

#pragma once

#include <boost/log/trivial.hpp>
#include "storage/Context.h"
#include <iostream>
#include <sys/stat.h>
#include <fstream>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte File (dossier racine), pour pouvoir récupérer des données stockées sous forme de fichiers
 */
class FileContext : public Context {
    
private:
    
    /**
     * \~french \brief Dossier racine des fichiers gérés
     * \details Le nom du fichier fourni pour les lectures et les écritures est concaténé à cette racine
     * \~english \brief Root directory for manipulated files
     * \details File name provided to read or write is concatenated with this root
     */
    std::string root_dir;

    /**
     * \~french \brief Flux d'écriture de l'image ROK4
     * \details Est ouvert avec #open_to_write et doit être fermé par #close_to_write
     * \~english \brief Stream used to write the ROK4 image
     * \details Is opened with #open_to_write and have to be closed with #close_to_write
     */
    std::ofstream output;

public:

    /**
     * \~french
     * \brief Constructeur pour un contexte Fichier
     * \param[in] root Répertoire des fichiers manipulés
     * \~english
     * \brief Constructor for File context
     * \param[in] root Directory for manipulated files
     */
    FileContext (std::string root);
    

    int read(uint8_t* data, int offset, int size, std::string name);
    uint8_t* read_full(int& size, std::string name);
    bool write(uint8_t* data, int offset, int size, std::string name);
    bool write_full(uint8_t* data, int size, std::string name);

    ContextType::eContextType get_type();
    std::string get_type_string();
    std::string get_tray();
 
    /**
     * \~french \brief Ouvre le flux #output
     * \~english \brief Open stream #output
     */
    bool open_to_write(std::string name) {
        std::string fullName = root_dir + name;
        output.open ( fullName.c_str(), std::ios_base::trunc | std::ios::binary );
        if (output.fail()) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * \~french \brief Ferme le flux #output
     * \~english \brief Close stream #output
     */
    bool close_to_write(std::string name) {
        output.close();
        if (output.fail()) {
            return false;
        } else {
            return true;
        }
    }


    std::string get_path(std::string racine,int x,int y,int pathDepth=2);
    std::string get_path(std::string name);


    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "------ File Context -------" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- root directory = " << root_dir ;
    }

    std::string to_string() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ File Context -------" << std::endl;
        oss << "\t- root directory = " << root_dir << std::endl;
        return oss.str() ;
    }
    
    bool connection();

    bool exists(std::string name);

    void close_connection() {
        connected = false;
    }
    
    ~FileContext() {
        close_connection();
    }
};


