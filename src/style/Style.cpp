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

/**
 * \file Style.cpp
 * \~french
 * \brief Implémentation de la classe Style modélisant les styles.
 * \~english
 * \brief Implement the Style Class handling style definition
 */

#include "style/Style.h"
#include <boost/log/trivial.hpp>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

bool Style::parse(json11::Json& doc, bool inspire) {

    /********************** Default values */

    pente = 0;
    aspect = 0;
    estompage = 0;
    palette = 0;
    usableForBroadcast = true;

    // Chargement

    if (doc["identifier"].is_string()) {
        identifier = doc["identifier"].string_value();
    } else {
        usableForBroadcast = false;
    }

    if (doc["title"].is_string()) {
        titles.push_back ( doc["title"].string_value() );
    }
    if (titles.size() == 0) {
        usableForBroadcast = false;
    }

    if (doc["abstract"].is_string()) {
        abstracts.push_back ( doc["abstract"].string_value() );
    }
    if ( abstracts.size() == 0 && inspire ) {
        errorMessage = "No abstract in style " + id + " : not INSPIRE compliant" ;
        return false;
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                errorMessage = "keywords have to be a string array";
                return false;
            }
        }
    }

    if (doc["legend"].is_object()) {
        LegendURL leg = LegendURL(doc["legend"].object_items());
        if (leg.getMissingField() != "") {
            errorMessage = "Invalid legend: have to own a field " + leg.getMissingField();
            return false;
        }
        legendURLs.push_back(leg);
    }

    if ( legendURLs.size() == 0 && inspire ) {
        errorMessage = "No legend in style " + id + " : not INSPIRE compliant" ;
        return false;
    }

    palette = new Palette(doc["palette"].object_items());
    if (! palette->isOk()) {
        errorMessage = "Palette issue for style " + id + ": " + palette->getErrorMessage();
        return false;
    }

    if (doc["estompage"].is_object()) {
        usableForBroadcast = false;
        estompage = new Estompage(doc["estompage"].object_items());
        if (! estompage->isOk()) {
            errorMessage = "Estompage issue for style " + id + ": " + estompage->getErrorMessage();
            return false;
        }
    }
    
    if ( estompage == 0 && doc["pente"].is_object()) {
        usableForBroadcast = false;
        pente = new Pente(doc["pente"].object_items());
        if (! pente->isOk()) {
            errorMessage = "Pente issue for style " + id + ": " + pente->getErrorMessage();
            return false;
        }
    }
    
    if ( estompage == 0 && pente == 0 && doc["exposition"].is_object()) {
        usableForBroadcast = false;
        aspect = new Aspect(doc["exposition"].object_items());
        if (! aspect->isOk()) {
            errorMessage = "Aspect issue for style " + id + ": " + aspect->getErrorMessage();
            return false;
        }
    }

    return true;
}

Style::Style ( std::string path, bool inspire ) : Configuration(path) {

    /********************** Id */
    id = Configuration::getFileName(filePath, ".json");
    BOOST_LOG_TRIVIAL(debug) << "Add style " << id << " from file";

    std::ifstream is(filePath);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON file "  + filePath + " : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc, inspire)) {
        return;
    }
}

Style::Style ( Style* obj) {

    id = obj->id;
    identifier = obj->identifier;
    titles= obj->titles;
    abstracts = obj->abstracts;
    keywords = obj->keywords;
    legendURLs = obj->legendURLs;
    usableForBroadcast = obj->usableForBroadcast;

    if (obj->pente != 0) {
        pente = new Pente(*obj->pente);
    }
    if (obj->estompage != 0) {
        estompage = new Estompage(*obj->estompage);
    }
    if (obj->palette != 0) {
        palette = new Palette(*obj->palette);
    }
    if (obj->aspect != 0) {
        aspect = new Aspect(*obj->aspect);
    }
}

Style::~Style() {
    if (pente != 0) {
        delete pente;
    }
    if (estompage != 0) {
        delete estompage;
    }
    if (palette != 0) {
        delete palette;
    }
    if (aspect != 0) {
        delete aspect;
    }
}
