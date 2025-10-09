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
 * \file StyleBook.cpp
 ** \~french
 * \brief Implémentation de la classe StyleBook
 ** \~english
 * \brief Implements classe StyleBook
 */


#include "rok4/utils/StyleBook.h"

StyleBook::StyleBook(){

}

void StyleBook::send_to_trash() {
    mtx.lock();
    std::map<std::string, Style*>::iterator it;
    for (it = book.begin(); it != book.end(); ++it) {
        trash.push_back(it->second);
    }
    book.clear();
    mtx.unlock();
}

void StyleBook::empty_trash() {
    mtx.lock();
    for (int i = 0; i < trash.size(); i++) {
        delete trash.at(i);
    }
    trash.clear();
    mtx.unlock();
}

void StyleBook::set_directory(std::string d) {
    // Suppression du slash final
    if (d.compare ( d.size()-1,1,"/" ) == 0) {
        d.pop_back();
    }
    directory = d;
}

std::map<std::string, Style *> StyleBook::get_book() {
    return book;
}

Style *StyleBook::get_style(std::string id) {
    std::map<std::string, Style*>::iterator it = book.find ( id );
    if ( it != book.end() ) {
        return it->second;
    }
    std::string d;
    if (directory.empty()) {
        char* e = getenv (ROK4_STYLES_DIRECTORY);
        if (e == NULL) {
            d.assign("/usr/share/rok4/styles");
        } else {
            d.assign(e);
        }
    } else {
        d = directory;
    }
    std::string style_path = d + "/" + id;
    mtx.lock();
    // Si on fait du cache de style, on revérifie que ce style n'a pas déjà été ajouté par un thread concurrent entre temps
    if(getenv (ROK4_STYLES_NO_CACHE) == NULL) {
        if ( it != book.end() ) {
            // On a effectivement depuis déjà enregistré ce style
            return it->second;
        }
    }
    Style* style = new Style(style_path);
    if ( ! style->is_ok() ) {
        BOOST_LOG_TRIVIAL(error) << style->get_error_message();
        delete style;
        mtx.unlock();
        return NULL;
    }
    if ( contain_chars(style->get_identifier(), "<>") ) {
        BOOST_LOG_TRIVIAL(error) << "Style identifier contains forbidden chars" ;
        delete style;
        mtx.unlock();
        return NULL;
    }
    if(getenv (ROK4_STYLES_NO_CACHE) == NULL) {
        // On veut utiliser le cache, on met donc ce nouveau style dans l'annuaire pour le trouver la porchaine fois
        book.insert ( std::pair<std::string, Style*>(id, style) );
    } else {
        // On met le style directement dans la corbeille, pour que le nettoyage se fasse bien
        trash.push_back(style);
    }
    mtx.unlock();
    return style;
}

int StyleBook::get_styles_count() {
    return book.size();
}

StyleBook::~StyleBook() {

}

std::map<std::string, Style*> StyleBook::book;
std::vector<Style*> StyleBook::trash;
std::string StyleBook::directory = "";
std::mutex StyleBook::mtx;