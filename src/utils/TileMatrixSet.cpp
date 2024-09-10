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
 * \file TileMatrix.cpp
 * \~french
 * \brief Implémentation de la classe TileMatrixSet gérant une pyramide de matrices (Cf TileMatrix)
 * \~english
 * \brief Implement the TileMatrixSet Class handling a pyramid of matrix (See TileMatrix)
 */


#include "utils/TileMatrixSet.h"
#include "utils/Utils.h"
#include "utils/Cache.h"
#include "storage/Context.h"

#include <cmath>
#include <fstream>

bool order_tm(TileMatrix* a, TileMatrix* b) 
{ 
    return (a->get_res() > b->get_res()); 
} 

bool TileMatrixSet::parse(json11::Json& doc) {

    // Récupération du CRS
    if (doc["crs"].is_string()) {
        crs = CrsBook::get_crs( doc["crs"].string_value() );
    } else {
        error_message = "crs have to be provided and be a string";
        return false;
    }
    

    // Récupération du titre
    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    }

    // Récupération du résumé
    if (doc["description"].is_string()) {
        abstract = doc["description"].string_value();
    }

    // Récupération des mots clés
    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            }
        }
    }

    if (doc["tileMatrices"].is_array()) {
        for (json11::Json tMat : doc["tileMatrices"].array_items()) {
            if (tMat.is_object()) {
                TileMatrix* tm = new TileMatrix(tMat.object_items());
                if (! tm->is_ok()) {
                    error_message = "tileMatrices contains an invalid level : " + tm->get_error_message();
                    delete tm;
                    return false;
                }
                tm_map.insert ( std::pair<std::string, TileMatrix*> ( tm->id, tm ) );
                tm_ordered.push_back(tm);
            } else {
                error_message = "tileMatrices have to be provided and be an object array";
                return false;
            }
        }
    } else {
        error_message = "tileMatrices have to be provided and be an object array";
        return false;
    }

    if ( tm_map.size() == 0 ) {
        error_message =  "No tile matrix in the Tile Matrix Set " + id ;
        return false;
    }

    std::sort(tm_ordered.begin(), tm_ordered.end(), order_tm); 

    return true;
}

TileMatrixSet::TileMatrixSet(std::string path) : Configuration(path) {

    crs = NULL;
    qtree = true;

    ContextType::eContextType storage_type;
    std::string tray_name, fo_name;
    ContextType::split_path(path, storage_type, fo_name, tray_name);

    /********************** Id */
    id = Configuration::get_filename(file_path, ".json");

    if ( contain_chars(id, "<>") ) {
        error_message =  "TileMatrixSet identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Add Tile Matrix Set " << id;

    /********************** Read */

    Context* context = StoragePool::get_context(storage_type, tray_name);
    if (context == NULL) {
        error_message = "Cannot add " + ContextType::to_string(storage_type) + " storage context to read TMS";
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
        error_message = "Cannot read TMS "  + path + ", with or without extension .json";
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
    
    // Détection des TMS Quad tree
    bool first = true;
    double res = 0;
    double x0 = 0;
    double y0 = 0;
    int tile_width = 0;
    int tile_height = 0;
    std::vector<TileMatrix*> bottom_to_top = tm_ordered;
    std::reverse(bottom_to_top.begin(), bottom_to_top.end());
    for (TileMatrix* tm : bottom_to_top) {
        if (first) {
            // Niveau du bas, de référence
            res = tm->get_res();
            x0 = tm->get_x0();
            y0 = tm->get_y0();
            tile_width = tm->get_tile_width();
            tile_height = tm->get_tile_height();
            first = false;
            continue;
        }
        if (abs(res * 2 - tm->get_res()) < 0.0001 * res && tm->get_x0() == x0 && tm->get_y0() == y0 && tm->get_tile_width() == tile_width && tm->get_tile_height() == tile_height) {
            res = tm->get_res();
        } else {
            qtree = false;
            break;
        }
    }

    return;
}

std::string TileMatrixSet::get_id() {
    return id;
}

std::vector<TileMatrix*> TileMatrixSet::get_ordered_tm(bool bottom_to_top) {
 
    if (bottom_to_top) {
        std::vector<TileMatrix*> levels = tm_ordered;
        std::reverse(levels.begin(),levels.end());
        return levels;
    } else {
        return tm_ordered;
    }

}

bool TileMatrixSet::operator== ( const TileMatrixSet& other ) const {
    return ( this->keywords.size() ==other.keywords.size()
             && this->tm_map.size() ==other.tm_map.size()
             && this->id.compare ( other.id ) == 0
             && this->title.compare ( other.title ) == 0
             && this->abstract.compare ( other.abstract ) == 0
             && this->crs==other.crs );
}

bool TileMatrixSet::operator!= ( const TileMatrixSet& other ) const {
    return ! ( *this == other );
}

TileMatrixSet::~TileMatrixSet() {
    std::map<std::string, TileMatrix*>::iterator it;
    for ( it=tm_map.begin(); it != tm_map.end(); it++ )
        delete it->second;
}

TileMatrix* TileMatrixSet::get_tm(std::string id) {

    std::map<std::string, TileMatrix*>::iterator it = tm_map.find ( id );

    if ( it == tm_map.end() ) {
        return NULL;
    }

    return it->second;
}

CRS* TileMatrixSet::get_crs() {
    return crs;
}

bool TileMatrixSet::is_qtree() {
    return qtree;
}

std::vector<Keyword>* TileMatrixSet::get_keywords() {
    return &keywords;
}

std::string TileMatrixSet::get_abstract() {
    return abstract;
}
std::string TileMatrixSet::get_title() {
    return title;
}

TileMatrix* TileMatrixSet::get_corresponding_tm(TileMatrix* tmIn, TileMatrixSet* tmsIn) {

    TileMatrix* tm = NULL;

    // on calcule la bbox géographique d'intersection des aires de définition des CRS des deux TMS, que l'on reprojete dans chaque CRS
    BoundingBox<double> bboxThis = get_crs()->get_crs_definition_area().get_intersection(tmsIn->get_crs()->get_crs_definition_area());
    BoundingBox<double> bboxIn = bboxThis;

    if (bboxThis.reproject(CRS::get_epsg4326(), get_crs()) && bboxIn.reproject(CRS::get_epsg4326(), tmsIn->get_crs())) {

        double ratio_x, ratio_y, resOutX, resOutY;
        double resIn = tmIn->get_res();

        ratio_x = (bboxThis.xmax - bboxThis.xmin) / (bboxIn.xmax - bboxIn.xmin);
        ratio_y = (bboxThis.ymax - bboxThis.ymin) / (bboxIn.ymax - bboxIn.ymin);

        resOutX = resIn * ratio_x;
        resOutY = resIn * ratio_y;

        double resolution = sqrt ( resOutX * resOutY );

        // On cherche le niveau du TMS le plus proche (ratio des résolutions le plus proche de 1)
        // On cherche un ration entre 0.8 et 1.5
        std::map<std::string, TileMatrix*>::iterator it = tm_map.begin();
        double ratio = 0;

        for ( ; it != tm_map.end(); it++ ) {
            double d = resolution / it->second->get_res();
            if (d < 0.8 || d > 1.5) {continue;}
            if (ratio == 0 || abs(d-1) < abs(ratio-1)) {
                ratio = d;
                tm = it->second;
            }
        }
    }

    return tm;
}