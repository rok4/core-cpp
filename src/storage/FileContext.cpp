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
 * \brief Implémentation de la classe FileContext
 * \details
 * \li FileContext : utilisation d'un système de fichier
 ** \~english
 * \brief Implement classe FileContext
 * \details
 * \li FileContext : file system use
 */

#include "storage/FileContext.h"
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <time.h>

using namespace std;

FileContext::FileContext (std::string root) : Context(), root_dir(root) {}

bool FileContext::connection() {
    connected = true;
    return true;
}

int FileContext::read(uint8_t* data, int offset, int size, std::string name) {
    std::string fullName = root_dir + name;
    BOOST_LOG_TRIVIAL(debug) << "File read : " << size << " bytes (from the " << offset << " one) in the file " << fullName;

    // Ouverture du fichier
    int fildes = open( fullName.c_str(), O_RDONLY );
    if ( fildes < 0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Can't open file " << fullName ;
        return -1;
    }

    size_t read_size = pread ( fildes, data, size, offset );

    if ( read_size != size ) {
        BOOST_LOG_TRIVIAL(error) <<  "Impossible de lire la tuile dans le fichier " << fullName ;
        if ( read_size<0 ) BOOST_LOG_TRIVIAL(error) <<  "Code erreur="<<errno ;
        close ( fildes );
        return -1;
    }

    close ( fildes );

    return read_size;
}


uint8_t* FileContext::read_full(int& size, std::string name) {
    size = -1;

    std::string fullName = root_dir + name;
    BOOST_LOG_TRIVIAL(debug) << "File read full : " << fullName;

    std::ifstream ifs(fullName);

    if (! ifs.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot open file " << fullName;
        return NULL;
    } else {

        ifs.seekg(0, std::ios::end);
        size = ifs.tellg();
        char* data = new char[size];
        memset(data, 0, size);

        ifs.seekg(0);
        ifs.read(data, size); 

        if ( size == 0 ) return NULL;
        return (uint8_t*) data;
    }
}


bool FileContext::write(uint8_t* data, int offset, int size, std::string name) {
    std::string fullName = root_dir + name;
    BOOST_LOG_TRIVIAL(debug) << "File write : " << size << " bytes (from the " << offset << " one) in the file " << fullName;

    if ( !output ) {
        BOOST_LOG_TRIVIAL(error) << "Not open output file " << fullName;
        return false;
    }

    output.seekp ( offset );
    if ( output.fail() ) return false;
    output.write ( ( char* ) data, size );
    if ( output.fail() ) return false;

    return true;
}

bool FileContext::write_full(uint8_t* data, int size, std::string name) {
    std::string fullName = root_dir + name;
    BOOST_LOG_TRIVIAL(debug) << "File write : " << size << " bytes (one shot) in the file " << fullName;

    if ( !output ) {
        BOOST_LOG_TRIVIAL(error) << "Not open output file " << fullName;
        return false;
    }

    output.write ( ( char* ) data, size );

    return true;
}

ContextType::eContextType FileContext::get_type() {
    return ContextType::FILECONTEXT;
}

std::string FileContext::get_type_string() {
    return "FILECONTEXT";
}

std::string FileContext::get_tray() {
    return root_dir;
}

/*
 * Tableau statique des caractères Base36 (pour systeme de fichier non case-sensitive)
 */
// static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const char* Base36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


std::string FileContext::get_path(std::string racine,int x,int y,int pathDepth){

    int pos;
    char path[32];
    path[sizeof ( path ) - 5] = '.';
    path[sizeof ( path ) - 4] = 't';
    path[sizeof ( path ) - 3] = 'i';
    path[sizeof ( path ) - 2] = 'f';
    path[sizeof ( path ) - 1] = 0;
    pos = sizeof ( path ) - 6;

    for ( int d = 0; d < pathDepth; d++ ) {
        path[pos--] = Base36[y % 36];
        path[pos--] = Base36[x % 36];
        path[pos--] = '/';
        x = x / 36;
        y = y / 36;
    }
    do {             
        path[pos--] = Base36[y % 36];
        path[pos--] = Base36[x % 36];
        x = x / 36;
        y = y / 36;
    } while ( x || y );
    path[pos] = '/';

    return racine + ( path + pos );

}


std::string FileContext::get_path(std::string name) {  
    return root_dir + name;
}

bool FileContext::exists(std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Exists (FILE) ? " << get_path(name);
    struct stat buffer;   
    return (stat (get_path(name).c_str(), &buffer) == 0);
}