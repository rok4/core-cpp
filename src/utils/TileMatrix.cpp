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

/*
 * TileMatrix.cpp
 */

/**
 * \file TileMatrix.cpp
 * \~french
 * \brief Implémentation de la classe TileMatrix gérant une matrice de tuile représentant un niveau d'une pyramide (Cf TileMatrixSet)
 * \~english
 * \brief Implement the TileMatrix Class handling a matrix of tile reprensenting a level in a pyramid (See TileMatrixSet)
 */

#include "utils/TileMatrix.h"
#include <cmath>

TileMatrix::TileMatrix(json11::Json doc) : Configuration() {

    if (doc["id"].is_string()) {
        id = doc["id"].string_value();
    } else {
        error_message = "id have to be provided and be a string";
        return;
    }
    if (doc["cellSize"].is_number()) {
        res = doc["cellSize"].number_value();
    } else {
        error_message = "cellSize have to be provided and be a number";
        return;
    }
    if (doc["tileWidth"].is_number()) {
        tile_width = doc["tileWidth"].number_value();
    } else {
        error_message = "tileWidth have to be provided and be a number";
        return;
    }
    if (doc["tileHeight"].is_number()) {
        tile_height = doc["tileHeight"].number_value();
    } else {
        error_message = "tileHeight have to be provided and be a number";
        return;
    }
    if (doc["matrixWidth"].is_number()) {
        matrix_width = doc["matrixWidth"].number_value();
    } else {
        error_message = "matrixWidth have to be provided and be a number";
        return;
    }
    if (doc["matrixHeight"].is_number()) {
        matrix_height = doc["matrixHeight"].number_value();
    } else {
        error_message = "matrixHeight have to be provided and be a number";
        return;
    }
    if (doc["pointOfOrigin"].is_array() && doc["pointOfOrigin"][0].is_number()) {
        x0 = doc["pointOfOrigin"][0].number_value();
    } else {
        error_message = "pointOfOrigin have to be provided and be a number array (2 items)";
        return;
    }
    if (doc["pointOfOrigin"].is_array() && doc["pointOfOrigin"][1].is_number()) {
        y0 = doc["pointOfOrigin"][1].number_value();
    } else {
        error_message = "pointOfOrigin have to be provided and be a number array (2 items)";
        return;
    }
}

double   TileMatrix::get_res()    {
    return res;
}
double   TileMatrix::get_x0()     {
    return x0;
}
double   TileMatrix::get_y0()     {
    return y0;
}
int      TileMatrix::get_tile_width()  {
    return tile_width;
}
int      TileMatrix::get_tile_height()  {
    return tile_height;
}
long int TileMatrix::get_matrix_width() {
    return matrix_width;
}
long int TileMatrix::get_matrix_height() {
    return matrix_height;
}

std::string TileMatrix::get_id()  {
    return id;
}

TileMatrix& TileMatrix::operator= ( const TileMatrix& other ) {

    this->id = other.id;
    this->res = other.res;
    this->x0 = other.x0;
    this->y0 = other.y0;
    this->tile_width = other.tile_width;
    this->tile_height = other.tile_height;
    this->matrix_width = other.matrix_width;
    this->matrix_height = other.matrix_height;

    return *this;
}


bool TileMatrix::operator== ( const TileMatrix& other ) const {
    return ( this->res == other.res
             && this->x0 == other.x0
             && this->y0 == other.y0
             && this->tile_height == other.tile_height
             && this->tile_width == other.tile_width
             && this->matrix_height == other.matrix_height
             && this->matrix_width == other.matrix_width
             && this->id.compare ( other.id ) ==0 );
}

bool TileMatrix::operator!= ( const TileMatrix& other ) const {
    return ! ( *this == other );
}

TileMatrix::~TileMatrix() { }

TileMatrixLimits TileMatrix::bbox_to_tile_limits(BoundingBox<double> bbox) {

    // On force les indices entre 0 et les max des matrix

    uint32_t min_tile_col = std::min(std::max(long((bbox.xmin - x0) / (tile_width * res)), 0L), matrix_width);
    uint32_t max_tile_col = std::min(std::max(long((bbox.xmax - x0) / (tile_width * res)), 0L), matrix_width);

    uint32_t min_tile_row = std::min(std::max(long((y0 - bbox.ymax) / (tile_height * res)), 0L), matrix_height);
    uint32_t max_tile_row = std::min(std::max(long((y0 - bbox.ymin) / (tile_height * res)), 0L), matrix_height);

    return TileMatrixLimits(id, min_tile_row, max_tile_row, min_tile_col, max_tile_col);
}


BoundingBox<double> TileMatrix::bbox_from_tile_limits(TileMatrixLimits limits) {

    // On force les indices entre 0 et les max des matrix

    double xmin = res * limits.get_min_tile_col() * tile_width + x0;
    double ymax = y0 - res * limits.get_min_tile_row() * tile_height ;
    double xmax = res * (limits.get_max_tile_col() + 1) * tile_width + x0;
    double ymin = y0 - res * (limits.get_max_tile_row() + 1) * tile_height;

    return BoundingBox<double>(xmin,ymin,xmax,ymax) ;
}

BoundingBox<double> TileMatrix::tile_indices_to_bbox (int col, int row) {

    double xmin = res * col * tile_width + x0;
    double ymax = y0 - res * row * tile_height ;
    double xmax = xmin + res * tile_width;
    double ymin = ymax - res * tile_height;

    return BoundingBox<double>(xmin,ymin,xmax,ymax) ;
}
