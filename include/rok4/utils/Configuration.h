/*
 * Copyright © (2011-2013) Institut national de l'information
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


#pragma once

#include <dirent.h>
#include <string>
#include <libgen.h>
#include <sys/stat.h>
#include <boost/log/trivial.hpp>

#include "rok4/thirdparty/json11.hpp"

class Configuration
{
    protected:

        std::string file_path;
        std::string file_directory;
        std::string error_message;
        Configuration() {error_message =  ""; file_path = ""; file_directory = "";}
        Configuration(std::string path) {
            error_message =  "";             
            
            file_path = path;

            char * fileNameChar = ( char * ) malloc ( strlen ( path.c_str() ) + 1 );
            strcpy ( fileNameChar, path.c_str() );
            char * parentDirChar = dirname ( fileNameChar );
            file_directory = std::string ( parentDirChar );
            free ( fileNameChar );
            fileNameChar=NULL;
            parentDirChar=NULL;
        }


        /**
        * \~french
        * \brief Destructeur par défaut
        * \~english
        * \brief Default destructor
        */
        ~Configuration(){};

    public:
        
        /**
         * \~french
         * \brief Précise si des erreurs ont été rencontrées
         * \return ok
         * \~english
         * \brief Precis if error occured
         * \return ok
         */
        bool is_ok() { return error_message == ""; }

        /**
         * \~french
         * \brief Retourne le message d'erreur
         * \return message d'erreur
         * \~english
         * \brief Return error message
         * \return error message
         */
        std::string get_error_message() { return error_message; }


        /**
         * \~french
         * \brief Récupère le nom d'un fichier
         * \~english
         * \brief Get file name
         */
        static std::string get_filename(std::string file, std::string extension) {

            std::string id;

            size_t idBegin=file.rfind ( "/" );
            if ( idBegin == std::string::npos ) {
                idBegin=0;
            }
            size_t idEnd=file.rfind ( extension );
            if ( idEnd == std::string::npos ) {
                idEnd=file.size();
            }
            id=file.substr ( idBegin+1, idEnd-idBegin-1 );

            return id;
        };


};


