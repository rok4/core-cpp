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

class Level;

#ifndef LEVEL_H
#define LEVEL_H

#include "rok4/image/Image.h"
#include "rok4/image/EmptyImage.h"
#include "rok4/utils/BoundingBox.h"
#include "rok4/utils/TileMatrix.h"
#include "rok4/utils/TileMatrixLimits.h"
#include "rok4/datasource/DataSource.h"
#include "rok4/utils/CRS.h"
#include "rok4/enums/Format.h"
#include "rok4/enums/Interpolation.h"
#include "rok4/storage/Context.h"
#include "rok4/utils/Pyramid.h"
#include "rok4/utils/Configuration.h"
#include "rok4/utils/Level.h"
#include "rok4/utils/Table.h"
#include "rok4/utils/Cache.h"

/**
 */

class Level : public Configuration{

friend class Pyramid;

private:

    // Stockage
    std::string racine;
    Context* context;
    int path_depth; // used only for file context

    // Format de stockage
    Rok4Format::eFormat format; // format d'image des tuiles
    uint32_t tiles_per_width; // nombre de tuiles par dalle dans le sens de la largeur
    uint32_t tiles_per_height;  // nombre de tuiles par dalle dans le sens de la hauteur

    // Quadrillage natif
    TileMatrix* tm;
    TileMatrixLimits tm_limits;

    /******************* PYRAMIDE VECTEUR *********************/
    std::vector<Table> tables;
    
    /******************** PYRAMIDE RASTER *********************/
    int channels;
    int* nodata_value;

    DataSource* get_encoded_tile ( int x, int y );
    DataSource* get_decoded_tile ( int x, int y );

protected:
    /**
     * Renvoie une image de taille width, height
     *
     * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tilex.
     * Toutes les coordonnées sont entière depuis le coin haut gauche.
     */
    Image* getwindow ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox<int64_t> src_bbox );

    Level ( json11::Json doc, Pyramid* pyramid, std::string path);
    Level ( Level* obj );

public:


    TileMatrix* get_tm() ;
    Rok4Format::eFormat get_format() ;
    int get_channels() ;

    TileMatrixLimits get_tile_limits() ;
    uint32_t get_max_tile_row() ;
    uint32_t get_min_tile_row() ;
    uint32_t get_max_tile_col() ;
    uint32_t get_min_tile_col() ;
    BoundingBox<double> get_bbox_from_tile_limits() ;
    void set_tile_limits_from_bbox(BoundingBox<double> bb) ;

    double get_res() ;
    std::string get_id() ;
    uint32_t get_tiles_per_width() ;
    uint32_t get_tiles_per_height() ;
    std::vector<Table>* get_tables() ;

    std::string get_path (int tilex, int tiley);
    Context* get_context() ;

    Image* getbbox ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox<double> bbox, int width, int height, Interpolation::KernelType interpolation );

    Image* getbbox ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox<double> bbox, int width, int height, CRS* src_crs, CRS* dst_crs, Interpolation::KernelType interpolation );
    /**
     * Renvoie la tuile x, y numéroté depuis l'origine.
     * Le coin haut gauche de la tuile (0,0) est (Xorigin, Yorigin)
     * Les indices de tuiles augmentes vers la droite et vers le bas.
     * Des indices de tuiles négatifs sont interdits
     *
     * La tuile contenant la coordonnées (X, Y) dans le srs d'origine a pour indice :
     * x = floor((X - X0) / (tile_width * resolution_x))
     * y = floor((Y - Y0) / (tile_height * resolution_y))
     */

    DataSource* get_tile (int x, int y);

    Image* get_tile ( int x, int y, int left, int top, int right, int bottom, bool null_for_nodata = false );

    /*
     * Destructeur
     */
    ~Level();

};

#endif





