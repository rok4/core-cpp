/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
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
 * \file Format.cpp
 ** \~french
 * \brief Implémentation du namespace ContextType
 * \details
 * \li ContextType : gère les types de contextes pour le stockage
 ** \~english
 * \brief Implement the namespace ContextType
 * \details
 * \li ContextType : managed context type for storage
 */

#include "storage/Context.h"
#include <string.h>

namespace ContextType {

/**
 * \~french \brief Noms des types de contextes
 * \~english \brief Available context type names
 */
const char *eContextTypeName[] = {
    "unknown",
    "file",
    "ceph",
    "swift",
    "s3"
};

std::string to_string ( eContextType ct ) {
    return std::string ( eContextTypeName[ct] );
}

eContextType from_string ( std::string strct ) {
    int i;
    for ( i = contexttype_size; i ; --i ) {
        if ( strct.compare ( eContextTypeName[i] ) == 0 )
            break;
    }
    return static_cast<eContextType> ( i );
}


void split_path(std::string path, ContextType::eContextType& type, std::string& fo, std::string& tray) {

    size_t pos = path.find ( "://" );
    if ( pos == std::string::npos ) {
        // Il n'y a pas de préfixe de stockage, on considère le mode fichier
        type = ContextType::FILECONTEXT;
    } else {
        std::string storage_type = path.substr ( 0, pos );
        type = ContextType::from_string(storage_type);
        if (type == ContextType::UNKNOWN) {
            type = ContextType::FILECONTEXT;
        }
        path = path.erase ( 0, storage_type.length() + 3);
    }


    if (type == ContextType::FILECONTEXT) {
        // Dans le cas d'un stockage fichier, le nom du fichier est l'ensemble et on ne définit pas de contenant
        fo = path;
        tray = "";
        return;
    }

    // Dans le cas d'un stockage objet, on sépare le contenant du nom de l'objet
    pos = path.find ( "/" );
    tray = path.substr ( 0, pos );
    fo = path.substr ( pos + 1, std::string::npos );

    return;
}

}

Context::Context () : connected(false) {

    char* e = getenv (ROK4_OBJECT_READ_ATTEMPTS);
    if (e == NULL || sscanf ( e, "%d", &read_attempts ) != 1 ) {
        read_attempts = 1;
    }

    e = getenv (ROK4_OBJECT_WRITE_ATTEMPTS);
    if (e == NULL || sscanf ( e, "%d", &write_attempts ) != 1 ) {
        write_attempts = 1;
    }

    e = getenv (ROK4_OBJECT_ATTEMPTS_WAIT);
    if (e == NULL || sscanf ( e, "%d", &waiting_time ) != 1 ) {
        waiting_time = 5;
    }
}
