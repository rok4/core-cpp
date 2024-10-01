/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file TileMatrixLimits.h
 * \~french
 * \brief Définition de la classe TileMatrixLimits gérant les indices extrêmes de tuiles pour un niveau
 * \~english
 * \brief Define the TileMatrixLimits class handling extrems tiles' indices for a level
 */

class TileMatrixLimits;

#pragma once

#include <string>
#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;

#include "rok4/utils/Configuration.h"

/**
 * \author Institut national de l'information géographique et forestière
 */
class TileMatrixLimits
{

public:
    /**
     * \~french
     * \brief Crée un TileMatrixLimits à partir des ses éléments constitutifs
     * \param[in] id Identifiant de niveau
     * \param[in] rmin Indice de ligne minimal
     * \param[in] rmax Indice de ligne maximal
     * \param[in] cmin Indice de colonne minimal
     * \param[in] cmax Indice de colonne maximal
     * \~english
     * \param[in] id layer
     * \param[in] rmin Min row indice
     * \param[in] rmax Max row indice
     * \param[in] cmin Min column indice
     * \param[in] cmax Max column indice
     */

    TileMatrixLimits(std::string id, uint32_t rmin, uint32_t rmax, uint32_t cmin, uint32_t cmax) {
        tm_id = id;
        min_tile_row = rmin;
        max_tile_row = rmax;
        min_tile_col = cmin;
        max_tile_col = cmax;
    };
    
    /**
     * \~french
     * \brief Crée un TileMatrixLimits vide
     * \~english
     * \brief Create an empty TileMatrixLimits
     */

    TileMatrixLimits() {};

    TileMatrixLimits(TileMatrixLimits const& other) {
        tm_id = other.tm_id;
        min_tile_row = other.min_tile_row;
        max_tile_row = other.max_tile_row;
        min_tile_col = other.min_tile_col;
        max_tile_col = other.max_tile_col;
    };

    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    TileMatrixLimits& operator= ( TileMatrixLimits const& other ) {
        if ( this != &other ) {
            this->tm_id = other.tm_id;
            this->min_tile_row = other.min_tile_row;
            this->max_tile_row = other.max_tile_row;
            this->min_tile_col = other.min_tile_col;
            this->max_tile_col = other.max_tile_col;
        }
        return *this;
    }

    ~TileMatrixLimits(){};

    uint32_t get_min_tile_row() {return min_tile_row;}
    uint32_t get_max_tile_row() {return max_tile_row;}
    uint32_t get_min_tile_col() {return min_tile_col;}
    uint32_t get_max_tile_col() {return max_tile_col;}

    bool contain_tile(int col, int row) {
        return (row >= min_tile_row && row <= max_tile_row && col >= min_tile_col && col <= max_tile_col);
    }

    /**
     * \~french \brief Identifiant de niveau
     * \~english \brief Level identifier
     */
    std::string tm_id;
    /**
     * \~french \brief Indice de ligne maximal
     * \~english \brief Max row indice
     */
    uint32_t max_tile_row;
    /**
     * \~french \brief Indice de ligne minimal
     * \~english \brief Min row indice
     */
    uint32_t min_tile_row;
    /**
     * \~french \brief Indice de colonne maximal
     * \~english \brief Max column indice
     */
    uint32_t max_tile_col;
    /**
     * \~french \brief Indice de colonne maximal
     * \~english \brief Min column indice
     */
    uint32_t min_tile_col;

    /**
     * \~french \brief Ajoute un noeud correpondant aux limites
     * \param[in] parent Noeud auquel ajouter celui des limites
     * \~english \brief Add a node corresponding to limits
     * \param[in] parent Node to whom add the limits node
     */
    void add_node(ptree& parent) {
        ptree& node = parent.add("TileMatrixLimits", "");

        node.add("TileMatrix", tm_id);
        node.add("MinTileRow", std::to_string(min_tile_row));
        node.add("MaxTileRow", std::to_string(max_tile_row));
        node.add("MinTileCol", std::to_string(min_tile_col));
        node.add("MaxTileCol", std::to_string(max_tile_col));
    }

    json11::Json to_json() const {
        json11::Json::object res = json11::Json::object {
            { "tileMatrix", tm_id },
            { "minTileRow", int(min_tile_row) },
            { "maxTileRow", int(max_tile_row) },
            { "minTileCol", int(min_tile_col) },
            { "maxTileCol", int(max_tile_col) }
        };
        
        return res;
    }

};



