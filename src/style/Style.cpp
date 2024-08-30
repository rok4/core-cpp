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
#include "utils/Cache.h"

#include "storage/Context.h"

bool Style::parse(json11::Json& doc) {

    /********************** Default values */

    pente = 0;
    aspect = 0;
    estompage = 0;
    palette = 0;

    input_nodata_value = NULL;
    output_nodata_value = NULL;

    // Chargement

    if (doc["identifier"].is_string()) {
        identifier = doc["identifier"].string_value();
    } else {
        error_message = "identifier have to be a string ";
        return false;
    }

    if (doc["title"].is_string()) {
        titles.push_back ( doc["title"].string_value() );
    } else {
        error_message = "title have to be a string ";
        return false;
    }

    if (doc["abstract"].is_string()) {
        abstracts.push_back ( doc["abstract"].string_value() );
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                error_message = "keywords have to be a string array";
                return false;
            }
        }
    }

    if (doc["legend"].is_object()) {
        LegendURL leg = LegendURL(doc["legend"].object_items());
        if (leg.get_missing_field() != "") {
            error_message = "Invalid legend: have to own a field " + leg.get_missing_field();
            return false;
        }
        legends.push_back(leg);
    }

    palette = new Palette(doc["palette"].object_items());
    if (! palette->is_ok()) {
        error_message = "Palette issue for style " + id + ": " + palette->get_error_message();
        return false;
    }

    if (doc["estompage"].is_object()) {
        estompage = new Estompage(doc["estompage"].object_items());
        if (! estompage->is_ok()) {
            error_message = "Estompage issue for style " + id + ": " + estompage->get_error_message();
            return false;
        }
    }
    
    if ( doc["pente"].is_object() ) {
        if (estompage != 0) {
            error_message = "Style " + id + " define estompage and pente rules";
            return false;
        }
        pente = new Pente(doc["pente"].object_items());
        if (! pente->is_ok()) {
            error_message = "Pente issue for style " + id + ": " + pente->get_error_message();
            return false;
        }
    }
    
    if ( doc["exposition"].is_object()) {
        if (estompage != 0 || pente != 0) {
            error_message = "Style " + id + " define exposition and estompage or pente rules";
            return false;
        }
        aspect = new Aspect(doc["exposition"].object_items());
        if (! aspect->is_ok()) {
            error_message = "Aspect issue for style " + id + ": " + aspect->get_error_message();
            return false;
        }
    }

    return true;
}

Style::Style ( std::string path ) : Configuration(path) {

    pente = 0;
    estompage = 0;
    palette = 0;
    aspect = 0;

    input_nodata_value = NULL;
    output_nodata_value = NULL;

    ContextType::eContextType storage_type;
    std::string tray_name, fo_name;
    ContextType::split_path(path, storage_type, fo_name, tray_name);

    /********************** Id */

    id = Configuration::get_filename(fo_name, ".json");
    BOOST_LOG_TRIVIAL(debug) << "Add style " << id << " from file or object";

    /********************** Read */

    Context* context = StoragePool::get_context(storage_type, tray_name);
    if (context == NULL) {
        error_message = "Cannot add " + ContextType::to_string(storage_type) + " storage context to read style";
        return;
    }

    int size = -1;
    uint8_t* data;

    // On supprime l'extension JSON si elle est dans le chemin, on testera avec dans un deuxième temps
    size_t pos = fo_name.rfind ( ".json" );
    if ( pos != std::string::npos ) {
        fo_name = fo_name.erase (pos, 5);
    }

    if (context->exists(fo_name)) {
        data = context->read_full(size, fo_name);
    } else if (context->exists(fo_name + ".json")) {
        data = context->read_full(size, fo_name + ".json");
    } else {
        error_message = "Cannot read style " + path + ", with or without extension .json";
        return;
    }

    std::string err;
    json11::Json doc = json11::Json::parse ( std::string((char*) data, size), err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON file "  + path + " : " + err ;
        return;
    }
    if (data != NULL) delete[] data;

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }


    // Input nodata
    if (estompage_defined()) {
        input_nodata_value = new int[1];
        input_nodata_value[0] = (int) estompage->input_nodata_value;
    }
    else if (aspect_defined()) {
        input_nodata_value = new int[1];
        input_nodata_value[0] = (int) aspect->input_nodata_value;
    }
    else if (pente_defined()) {
        input_nodata_value = new int[1];
        input_nodata_value[0] = (int) pente->input_nodata_value;
    } 
    else if (palette && ! palette->is_empty()) {
        input_nodata_value = new int[1];
        input_nodata_value[0] = (int) palette->get_colours_map()->begin()->first;
    }

    // Output nodata
    if (palette && ! palette->is_empty()) {
        Colour c = palette->get_colours_map()->begin()->second;
        if (palette->is_no_alpha()) {
            output_nodata_value = new int[3];
            output_nodata_value[0] = c.r;
            output_nodata_value[1] = c.g;
            output_nodata_value[2] = c.b;
        } else {
            output_nodata_value = new int[4];
            output_nodata_value[0] = c.r;
            output_nodata_value[1] = c.g;
            output_nodata_value[2] = c.b;
            output_nodata_value[3] = c.a;
        }
    }
    else if (estompage_defined()) {
            output_nodata_value = new int[1];
            output_nodata_value[0] = (int) estompage->estompage_nodata_value;
    }
    else if (aspect_defined()) {
        output_nodata_value = new int[1];
        output_nodata_value[0] = (int) aspect->aspect_nodata_value;
    }
    else if (pente_defined()) {
        output_nodata_value = new int[1];
        output_nodata_value[0] = (int) pente->slope_nodata_value;
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
    if (input_nodata_value != NULL) {
        delete[] input_nodata_value;
    }
    if (output_nodata_value != NULL) {
        delete[] output_nodata_value;
    }
}

void Style::add_node_wmts(ptree& parent, bool default_style) {
    ptree& node = parent.add("Style", "");
    if ( default_style ) {
        node.add("<xmlattr>.isDefault", "true");
    }
    
    for (std::string t : titles) {
        node.add("Title", t);
    }
    for (std::string a : abstracts) {
        node.add("Abstract", a);
    }

    if ( keywords.size() != 0 ) {
        ptree& keywords_node = node.add("ows:Keywords", "");
        for (Keyword k  : keywords) {
            k.add_node(keywords_node, "ows:Keyword");
        }
    }

    node.add("ows:Identifier", identifier);
    
    for (LegendURL l : legends) {
        l.add_node_wmts(node);
    }
}

void Style::add_node_wms(ptree& parent) {
    ptree& node = parent.add("Style", "");

    node.add("Name", identifier);

    for (std::string t : titles) {
        node.add("Title", t);
    }
    for (std::string a : abstracts) {
        node.add("Abstract", a);
    }
    for (LegendURL l : legends) {
        l.add_node_wms(node);
    }
}