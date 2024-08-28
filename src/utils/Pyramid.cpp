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

#include <cmath>
#include "utils/LegendURL.h"
#include <boost/log/trivial.hpp>
#include "processors/Grid.h"
#include "datasource/Decoder.h"
#include "datastream/JPEGEncoder.h"
#include "datastream/PNGEncoder.h"
#include "datastream/TiffEncoder.h"
#include "datastream/BilEncoder.h"
#include "datastream/AscEncoder.h"
#include "image/ExtendedCompoundImage.h"
#include "enums/Format.h"
#include "utils/Level.h"
#include "utils/Cache.h"
#include <cfloat>
#include "image/EmptyImage.h"

ComparatorLevel compLevelDesc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->get_res() > elem2.second->get_res();
    };

ComparatorLevel compLevelAsc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->get_res() < elem2.second->get_res();
    };


bool Pyramid::parse(json11::Json& doc) {

    // TMS
    std::string tmsName;
    if (doc["tile_matrix_set"].is_string()) {
        tmsName = doc["tile_matrix_set"].string_value();
    } else {
        error_message = "tile_matrix_set have to be provided and be a string";
        return false;
    }

    tms = TmsBook::get_tms(tmsName);
    if ( tms == NULL ) {
        error_message =  "Pyramid use unknown or unloadable TMS [" + tmsName + "]" ;
        return false;
    }

    // FORMAT
    std::string formatStr;
    if (doc["format"].is_string()) {
        formatStr = doc["format"].string_value();
    } else {
        error_message = "format have to be provided and be a string";
        return false;
    }

    format = Rok4Format::from_string ( formatStr );
    if ( ! ( format ) ) {
        error_message =  "Le format [" + formatStr + "] n'est pas gere." ;
        return false;
    }

    /******************* PYRAMIDE RASTER *********************/
    
    if (Rok4Format::is_raster(format)) {
        if (! doc["raster_specifications"].is_object()) {
            error_message = "raster_specifications have to be provided and be an object for raster format";
            return false;
        }

        // PHOTOMETRIE
        std::string photometricStr;
        if (doc["raster_specifications"]["photometric"].is_string()) {
            photometricStr = doc["raster_specifications"]["photometric"].string_value();
        } else {
            error_message = "raster_specifications.photometric have to be provided and be a string";
            return false;
        }

        photo = Photometric::from_string ( photometricStr );
        if ( ! ( photo ) ) {
            error_message =  "La photométrie [" + photometricStr + "] n'est pas gere." ;
            return false;
        }

        // CHANNELS
        if (doc["raster_specifications"]["channels"].is_number()) {
            channels = doc["raster_specifications"]["channels"].number_value();
        } else {
            error_message = "raster_specifications.channels have to be provided and be an integer";
            return false;
        }

        // NODATAVALUE
        nodata_value = new int[channels];
        if (doc["raster_specifications"]["nodata"].is_string()) {
            std::string nodataValueStr = doc["raster_specifications"]["nodata"].string_value();
            std::size_t found = nodataValueStr.find_first_of(",");
            std::string currentValue = nodataValueStr.substr(0,found);
            std::string endOfValues = nodataValueStr.substr(found+1);
            int curVal = atoi(currentValue.c_str());
            if (currentValue == "") {
                curVal = DEFAULT_NODATAVALUE;
            }
            int i = 0;
            nodata_value[i] = curVal;
            i++;
            while (found!=std::string::npos && i < channels) {
                found = endOfValues.find_first_of(",");
                currentValue = endOfValues.substr(0,found);
                endOfValues = endOfValues.substr(found+1);
                curVal = atoi(currentValue.c_str());
                if (currentValue == "") {
                    curVal = DEFAULT_NODATAVALUE;
                }
                nodata_value[i] = curVal;
                i++;
            }
            if (i < channels) {
                error_message =  "channels is greater than the count of value for nodata";
                return false;
            }
        } else {
            error_message = "raster_specifications.nodata have to be provided and be a string";
            return false;
        }
    }

    /******************* PARTIE COMMUNE *********************/

    // LEVELS
    if (doc["levels"].is_array()) {
        for (json11::Json l : doc["levels"].array_items()) {
            if (l.is_object()) {
                Level* level = new Level(l, this, file_path);
                if ( ! level->is_ok() ) {
                    error_message = "levels contains an invalid level : " + level->get_error_message();
                    delete level;
                    return false;
                }

                //on va vérifier que le level qu'on vient de charger n'a pas déjà été chargé
                std::map<std::string, Level*>::iterator it = levels.find ( level->get_id() );
                if ( it != levels.end() ) {
                    error_message =  "Level " + level->get_id() + " defined twice" ;
                    delete level;
                    return false;
                }

                levels.insert ( std::pair<std::string, Level*> ( level->get_id(), level ) );
            } else {
                error_message = "levels have to be provided and be an object array";
                return false;
            }
        }
    } else {
        error_message = "levels have to be provided and be an object array";
        return false;
    }

    if ( levels.size() == 0 ) {
        error_message = "No level loaded";
        return false;
    }

    return true;
}

Pyramid::Pyramid(std::string path) : Configuration(path) {

    nodata_value = NULL;

    /********************** Read */

    ContextType::eContextType storage_type;
    std::string tray_name, fo_name;
    ContextType::split_path(path, storage_type, fo_name, tray_name);
    
    context = StoragePool::get_context(storage_type, tray_name);
    if (context == NULL) {
        error_message = "Cannot add " + ContextType::to_string(storage_type) + " storage context to read pyramid's descriptor";
        return;
    }

    int size = -1;
    uint8_t* data = context->read_full(size, fo_name);

    if (size < 0) {
        error_message = "Cannot read descriptor "  + path ;
        if (data != NULL) delete[] data;
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

    std::map<std::string, Level*>::iterator itLevel;
    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel=levels.begin(); itLevel!=levels.end(); itLevel++ ) {

        //Determine Higher and Lower Levels
        double d = itLevel->second->get_res();
        if ( minRes > d ) {
            minRes = d;
            lowest_level = itLevel->second;
        }
        if ( maxRes < d ) {
            maxRes = d;
            highest_level = itLevel->second;
        }
    }
}

Pyramid::Pyramid (Pyramid* obj) {
    tms = obj->tms;
    format = obj->format;
    lowest_level = NULL;
    highest_level = NULL;
    nodata_value = NULL;

    if (Rok4Format::is_raster(format)) {
        photo = obj->photo;
        channels = obj->channels;

        nodata_value = new int[channels];
        memcpy ( nodata_value, obj->nodata_value, channels * sizeof(int) );
    }
}

Context* Pyramid::get_context() { return context; }

bool Pyramid::add_levels (Pyramid* obj, std::string bottomLevel, std::string topLevel) {

    // Caractéristiques globales
    if (tms->get_id() != obj->tms->get_id()) {
        BOOST_LOG_TRIVIAL(error) << "TMS have to be the same for all used pyramids";
        return false;
    }
    if (format != obj->format) {
        BOOST_LOG_TRIVIAL(error) << "Format have to be the same for all used pyramids";
        return false;
    }

    if (Rok4Format::is_raster(format)) {
        if (photo != obj->photo) {
            BOOST_LOG_TRIVIAL(error) << "Photometric have to be the same for all used pyramids";
            return false;
        }
        if (channels != obj->channels) {
            BOOST_LOG_TRIVIAL(error) << "Channels count have to be the same for all used pyramids";
            return false;
        }
    }

    // Niveaux
    bool begin = false;
    bool end = false;
    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = obj->get_ordered_levels(true);
    for (std::pair<std::string, Level*> element : orderedLevels) {
        std::string levelId = element.second->get_id();
        if (! begin && levelId != bottomLevel) {
            continue;
        }
        begin = true;

        if (get_level(levelId) != NULL) {
            BOOST_LOG_TRIVIAL(error) << "Level " << levelId << " is already present"  ;
            return false;
        }

        Level* l = new Level(element.second);
        levels.insert ( std::pair<std::string, Level*> ( levelId, l ) );

        if (lowest_level == NULL || l->get_res() < lowest_level->get_res()) {
            lowest_level = l;
        }
        if (highest_level == NULL || l->get_res() > highest_level->get_res()) {
            highest_level = l;
        }

        if (levelId == topLevel) {
            end = true;
            break;
        }
    }

    if (! begin) {
        BOOST_LOG_TRIVIAL(error) << "Bottom level " << bottomLevel << " not found in the input pyramid"  ;
        return false;
    }

    if (! end) {
        BOOST_LOG_TRIVIAL(error) << "Top level " << topLevel << " not found in the input pyramid or lower than the bottom level"  ;
        return false;
    }

    return true;
}

std::string Pyramid::best_level ( double resolution_x, double resolution_y ) {

    // TODO: A REFAIRE !!!!
    // res_level/resx ou resy ne doit pas exceder une certaine valeur
    double resolution = sqrt ( resolution_x * resolution_y );

    std::map<std::string, Level*>::iterator it ( levels.begin() ), itend ( levels.end() );
    std::string best_h = it->first;
    double best = resolution_x / it->second->get_res();
    ++it;
    for ( ; it!=itend; ++it ) {
        double d = resolution / it->second->get_res();
        if ( ( best < 0.8 && d > best ) ||
                ( best >= 0.8 && d >= 0.8 && d < best ) ) {
            best = d;
            best_h = it->first;
        }
    }
    return best_h;
}


Image* Pyramid::getbbox ( unsigned int max_tile_x, unsigned int max_tile_y, BoundingBox<double> bbox, int width, int height, CRS* dst_crs, bool crs_equals, Interpolation::KernelType interpolation, int dpi ) {

    // On calcule la résolution de la requete dans le crs source selon une diagonale de l'image
    double resolution_x, resolution_y;

    BOOST_LOG_TRIVIAL(debug) << "Reprojection " << tms->get_crs()->get_proj_code() << " -> " << dst_crs->get_proj_code() ;

    if ( crs_equals ) {
        resolution_x = ( bbox.xmax - bbox.xmin ) / width;
        resolution_y = ( bbox.ymax - bbox.ymin ) / height;
    } else {
        BoundingBox<double> tmp = bbox;

        if ( ! tmp.reproject ( dst_crs, tms->get_crs() ) ) {
            // BBOX invalide

            BOOST_LOG_TRIVIAL(warning) << "reproject en erreur" ;
            return NULL;
        }

        resolution_x = ( tmp.xmax - tmp.xmin ) / width;
        resolution_y = ( tmp.ymax - tmp.ymin ) / height;
    }

    if (dpi != 0) {
        //si un parametre dpi a ete donne dans la requete, alors on l'utilise
        resolution_x = resolution_x * dpi / 90.7;
        resolution_y = resolution_y * dpi / 90.7;
        //on teste si on vient d'avoir des NaN
        if (resolution_x != resolution_x || resolution_y != resolution_y) {
            return NULL;
        }
    }

    std::string l = best_level ( resolution_x, resolution_y );
    BOOST_LOG_TRIVIAL(debug) <<  "best_level=" << l <<" resolution requete=" << resolution_x << " " << resolution_y  ;

    if ( crs_equals ) {
        return levels[l]->getbbox ( max_tile_x, max_tile_y, bbox, width, height, interpolation );
    } else {
        return create_reprojected_image(l, bbox, dst_crs, max_tile_x, max_tile_y, width, height, interpolation);
    }

}

Image* Pyramid::create_reprojected_image(std::string l, BoundingBox<double> bbox, CRS* dst_crs, unsigned int max_tile_x, unsigned int max_tile_y, int width, int height, Interpolation::KernelType interpolation) {

    bbox.crs = dst_crs->get_request_code();

    if (bbox.is_in_crs_area(dst_crs)) {
        // La bbox entière de l'image demandée est dans l'aire de définition du CRS cible
        return levels[l]->getbbox ( max_tile_x, max_tile_y, bbox, width, height, tms->get_crs(), dst_crs, interpolation );

    } else if (bbox.intersect_crs_area(dst_crs)) {
        // La bbox n'est pas entièrement dans l'aire du CRS, on doit faire la projection que sur la partie intérieure

        BoundingBox<double> croped = bbox.crop_to_crs_area(dst_crs);

        double resx = (bbox.xmax - bbox.xmin) / width;
        double resy = (bbox.ymax - bbox.ymin) / height;
        croped.phase(bbox, resx, resy);

        if (croped.has_null_area()) {
            BOOST_LOG_TRIVIAL(debug) <<   "BBox decoupée d'aire nulle"  ;
            EmptyImage* fond = new EmptyImage(width, height, channels, nodata_value);
            fond->set_bbox(bbox);
            return fond;
        }

        int croped_width = int ( ( croped.xmax - croped.xmin ) / resx + 0.5 );
        int croped_height = int ( ( croped.ymax - croped.ymin ) / resy + 0.5 );

        std::vector<Image*> images;
        Image* tmp = levels[l]->getbbox ( max_tile_x, max_tile_y, croped, croped_width, croped_height, tms->get_crs(), dst_crs, interpolation );
        if ( tmp != 0 ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Image decoupée valide"  ;
            images.push_back ( tmp );
        } else {
            BOOST_LOG_TRIVIAL(error) <<   "Image decoupée non valide"  ;
            EmptyImage* fond = new EmptyImage(width, height, channels, nodata_value);
            fond->set_bbox(bbox);
            return fond;
        }

        return ExtendedCompoundImage::create ( width, height, channels, bbox, images, nodata_value, 0 );
        
    } else {

        BOOST_LOG_TRIVIAL(error) <<  "La bbox de l'image demandée est totalement en dehors de l'aire de définition du CRS de destination " << dst_crs->get_proj_code() ;
        BOOST_LOG_TRIVIAL(error) <<  bbox.to_string() ;
        return 0;
    }
}

Pyramid::~Pyramid() {

    if (nodata_value != NULL) delete[] nodata_value;

    std::map<std::string, Level*>::iterator iLevel;
    for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
        delete iLevel->second;

}

Compression::eCompression Pyramid::get_sample_compression() {
    return Rok4Format::get_compression(format);
}

SampleFormat::eSampleFormat Pyramid::get_sample_format() {
    return Rok4Format::get_sample_format(format);
}

Level* Pyramid::get_highest_level() { return highest_level; }
Level* Pyramid::get_lowest_level() { return lowest_level; }

TileMatrixSet* Pyramid::get_tms() { return tms; }
std::map<std::string, Level*>& Pyramid::get_levels() { return levels; }

std::set<std::pair<std::string, Level*>, ComparatorLevel> Pyramid::get_ordered_levels(bool asc) {
 
    if (asc) {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelAsc);
    } else {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelDesc);
    }

}

Level* Pyramid::get_level(std::string id) {
    std::map<std::string, Level*>::iterator it= levels.find ( id );
    if ( it == levels.end() ) {
        return NULL;
    }
    return it->second;
}

Rok4Format::eFormat Pyramid::get_format() { return format; }
Photometric::ePhotometric Pyramid::get_photometric() { return photo; }
int Pyramid::get_channels() { return channels; }
int* Pyramid::get_nodata_value() { return nodata_value; }
