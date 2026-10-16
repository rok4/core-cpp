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
 * \file Exception.h
 * \~french
 * \brief Définition de la gestion des exceptions
 * \~english
 * \brief Define the exceptions handler
 */

#pragma once

using namespace std;

#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Erreur de stockage
 */
class StorageException : public exception {
   private:
    string message;

   public:
    // Constructor accepting const char*
    StorageException(const char* msg) : message(msg) {}

    // Override what() method, marked
    // noexcept for modern C++
    const char* what() const noexcept {
        return message.c_str();
    }
};

class NotFoundStorageException : public StorageException {
   public:
    NotFoundStorageException(std::string msg) : StorageException(msg.c_str()) {}
};

class UnreachableStorageException : public StorageException {
   public:
    UnreachableStorageException(std::string msg) : StorageException(msg.c_str()) {}
};

class ForbiddenStorageException : public StorageException {
   public:
    ForbiddenStorageException(std::string msg) : StorageException(msg.c_str()) {}
};