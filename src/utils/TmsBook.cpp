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
 * \file TmsBook.cpp
 ** \~french
 * \brief Implémentation de la classe TmsBook
 ** \~english
 * \brief Implements classe TmsBook
 */


#include "rok4/utils/TmsBook.h"

TmsBook::TmsBook(){

}

void TmsBook::send_to_trash() {
    mtx.lock();
    std::map<std::string, TileMatrixSet*>::iterator it;
    for (it = book.begin(); it != book.end(); ++it) {
        trash.push_back(it->second);
    }
    book.clear();
    mtx.unlock();
}

void TmsBook::empty_trash() {
    mtx.lock();
    for (int i = 0; i < trash.size(); i++) {
        delete trash.at(i);
    }
    trash.clear();
    mtx.unlock();
}

void TmsBook::set_directory(std::string d) {
    // Suppression du slash final
    if (d.compare ( d.size()-1,1,"/" ) == 0) {
        d.pop_back();
    }
    directory = d;
}

std::map<std::string, TileMatrixSet *> TmsBook::get_book() {
    return book;
}

TileMatrixSet *TmsBook::get_tms(std::string id) {
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
    if ( ! tms->is_ok() ) {
        BOOST_LOG_TRIVIAL(error) << tms->get_error_message();
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

int TmsBook::get_tms_count() {
    return book.size();
}

TmsBook::~TmsBook(){

}

std::map<std::string, TileMatrixSet*> TmsBook::book;
std::vector<TileMatrixSet*> TmsBook::trash;
std::string TmsBook::directory = "";
std::mutex TmsBook::mtx;