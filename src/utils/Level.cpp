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

#include "utils/Level.h"
#include "enums/Interpolation.h"
#include "datasource/StoreDataSource.h"
#include "image/CompoundImage.h"
#include "image/ResampledImage.h"
#include "image/ReprojectedImage.h"
#include "image/RawImage.h"
#include "datasource/Decoder.h"
#include "datastream/TiffEncoder.h"
#include "datasource/TiffHeaderDataSource.h"
#include <cmath>
#include <boost/log/trivial.hpp>
#include "processors/Kernel.h"
#include <vector>
#include "storage/Context.h"
#include "storage/FileContext.h"
#include "storage/S3Context.h"
#include "storage/SwiftContext.h"
#include "datasource/PaletteDataSource.h"
#include "enums/Format.h"
#include <cstddef>
#include <sys/stat.h>

#if CEPH_ENABLED
#include "storage/ceph/CephPoolContext.h"
#endif


#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.

Level::Level ( json11::Json doc, Pyramid* pyramid, std::string path) : Configuration(path) {
    nodata_value = NULL;

    // Copie d'informations depuis la pyramide
    format = pyramid->get_format();

    if (Rok4Format::is_raster(format)) {
        channels = pyramid->get_channels();        
        nodata_value = new int[channels];
        memcpy ( nodata_value, pyramid->get_nodata_value(), channels * sizeof(int) );
    } else {
        channels = 0;
    }

    context = NULL;

    // TM
    if (! doc["id"].is_string()) {
        error_message = "id have to be provided and be a string";
        return;
    }
    std::string id = doc["id"].string_value();
    TileMatrixSet* tms = pyramid->get_tms();
    tm = tms->get_tm(id);
    if ( tm == NULL ) {
        error_message = "Level " + id + " not in the pyramid TMS [" + tms->get_id() + "]"  ;
        return;
    }

    // STOCKAGE
    if (! doc["storage"].is_object()) {
        error_message = "Level " + id +": storage have to be provided and be an object";
        return;
    }
    if (! doc["storage"]["type"].is_string()) {
        error_message = "Level " + id +": storage.type have to be provided and be a string";
        return;
    }

    /******************* STOCKAGE FICHIER ? *********************/

    if (doc["storage"]["type"].string_value() == "FILE") {

        if (! doc["storage"]["image_directory"].is_string()) {
            error_message = "Level " + id +": storage.image_directory have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["path_depth"].is_number()) {
            error_message = "Level " + id +": storage.path_depth have to be provided and be an integer";
            return;
        }

        // En mode fichier, le chemin est potentiellement fourni avec un préfixe file:// qui ne doit pas être repris
        ContextType::eContextType storage_type;
        std::string tray_name, fo_name;
        ContextType::split_path(path, storage_type, fo_name, tray_name);

        char * fileNameChar = ( char * ) malloc ( strlen ( fo_name.c_str() ) + 1 );
        strcpy ( fileNameChar, fo_name.c_str() );
        char * parentDirChar = dirname ( fileNameChar );
        std::string parent = std::string ( parentDirChar );
        free ( fileNameChar );

        racine = doc["storage"]["image_directory"].string_value() ;
        //Relative Path
        if ( racine.compare ( 0, 2, "./" ) == 0 ) {
            racine.replace ( 0, 1, parent );
        } else if ( racine.compare ( 0, 1, "/" ) != 0 ) {
            racine.insert ( 0,"/" );
            racine.insert ( 0, parent );
        }

        path_depth = doc["storage"]["path_depth"].number_value();

        context = StoragePool::get_context(ContextType::FILECONTEXT, "");
        if (context == NULL) {
            error_message = "Level " + id +": cannot add file storage context";
            return;
        }
    }
    /******************* STOCKAGE OBJET ? *********************/

    else if (doc["storage"]["type"].string_value() == "SWIFT") {

        if (! doc["storage"]["container_name"].is_string()) {
            error_message = "Level " + id +": storage.container_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            error_message = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        context = StoragePool::get_context(ContextType::SWIFTCONTEXT, doc["storage"]["container_name"].string_value());
        if (context == NULL) {
            error_message = "Level " + id +": cannot add swift storage context";
            return;
        }
    }
    else if (doc["storage"]["type"].string_value() == "S3") {

        if (! doc["storage"]["bucket_name"].is_string()) {
            error_message = "Level " + id +": storage.bucket_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            error_message = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        // On fournit le contexte de stockage de la pyramide. Cela permet de s'assurer que celui du niveau est sur le même cluster S3
        context = StoragePool::get_context(ContextType::S3CONTEXT, doc["storage"]["bucket_name"].string_value(), pyramid->get_context());
        if (context == NULL) {
            error_message = "Level " + id +": cannot add s3 storage context";
            return;
        }
    }
#if CEPH_ENABLED
    else if (doc["storage"]["type"].string_value() == "CEPH") {

        if (! doc["storage"]["pool_name"].is_string()) {
            error_message = "Level " + id +": storage.pool_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            error_message = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        context = StoragePool::get_context(ContextType::CEPHCONTEXT, doc["storage"]["pool_name"].string_value());
        if (context == NULL) {
            error_message = "Level " + id +": cannot add ceph storage context";
            return;
        }
    }
#endif

    if (context == NULL) {
        error_message = "Level " + id + " without valid storage informations" ;
        return;
    }

    /******************* PYRAMIDE VECTEUR *********************/
    if (doc["tables"].is_array()) {
        for (json11::Json t : doc["tables"].array_items()) {
            if (t.is_object()) {
                if (! t["name"].is_string()) {
                    error_message = "Level " + id +": tables element have to own a name";
                    return;
                }
                std::string tableName  = t["name"].string_value();

                if (! t["geometry"].is_string()) {
                    error_message = "Level " + id +": tables element have to own a geometry";
                    return;
                }
                std::string geometry  = t["geometry"].string_value();

                std::vector<Attribute> atts;

                if (t["attributes"].is_array()) {
                    for (json11::Json a : t["attributes"].array_items()) {
                        if (a.is_object()) {
                            Attribute att = Attribute(a);
                            if (att.get_missing_field() != "") {
                                error_message = "Level " + id +": tables.attributes have to own a field " + att.get_missing_field();
                                return;
                            }
                            atts.push_back(att);
                        } else {
                            error_message = "Level " + id +": tables.attributes have to be an object array";
                            return;
                        }
                    }
                } else {
                    error_message = "Level " + id +": tables.attributes have to be an object array";
                    return;
                }

                tables.push_back(Table(tableName, geometry, atts));
            } else {
                error_message = "Level " + id +": tables have to be provided and be an object array";
                return;
            }
        }
    }

    /******************* PARTIE COMMUNE *********************/

    // TILEPERWIDTH

    if (! doc["tiles_per_width"].is_number()) {
        error_message = "Level " + id +": tiles_per_width have to be provided and be an integer";
        return;
    }
    tiles_per_width = doc["tiles_per_width"].number_value();

    // TILEPERHEIGHT

    if (! doc["tiles_per_height"].is_number()) {
        error_message = "Level " + id +": tiles_per_height have to be provided and be an integer";
        return;
    }
    tiles_per_height = doc["tiles_per_height"].number_value();

    if (tiles_per_height == 0 || tiles_per_width == 0) {
        error_message = "Level " + id +": slab tiles size have to be non zero integers"  ;
        return;
    }

    // TMSLIMITS

    if (! doc["tile_limits"].is_object()) {
        error_message = "Level " + id +": tile_limits have to be provided and be an object";
        return;
    }
    if (! doc["tile_limits"]["min_row"].is_number()) {
        error_message = "Level " + id +": tile_limits.min_row have to be provided and be an integer";
        return;
    }
    int min_tile_row = doc["tile_limits"]["min_row"].number_value();

    if (! doc["tile_limits"]["min_col"].is_number()) {
        error_message = "Level " + id +": tile_limits.min_col have to be provided and be an integer";
        return;
    }
    int min_tile_col = doc["tile_limits"]["min_col"].number_value();

    if (! doc["tile_limits"]["max_row"].is_number()) {
        error_message = "Level " + id +": tile_limits.max_row have to be provided and be an integer";
        return;
    }
    int max_tile_row = doc["tile_limits"]["max_row"].number_value();

    if (! doc["tile_limits"]["max_col"].is_number()) {
        error_message = "Level " + id +": tile_limits.max_col have to be provided and be an integer";
        return;
    }
    int max_tile_col = doc["tile_limits"]["max_col"].number_value();

    if ( min_tile_col > tm->get_matrix_width() || min_tile_col < 0 ) min_tile_col = 0;
    if ( min_tile_row > tm->get_matrix_height() || min_tile_row < 0 ) min_tile_row = 0;
    if ( max_tile_col > tm->get_matrix_width() || max_tile_col < 0 ) max_tile_col = tm->get_matrix_width();
    if ( max_tile_row > tm->get_matrix_height() || max_tile_row < 0 ) max_tile_row = tm->get_matrix_height();

    tm_limits = TileMatrixLimits(id, min_tile_row, max_tile_row, min_tile_col, max_tile_col);

    return;
}


Level::Level ( Level* obj ) : Configuration(obj->file_path), tm_limits(obj->tm_limits) {
    nodata_value = NULL;
    tm = obj->tm;

    channels = obj->channels;
    racine = obj->racine;

    tiles_per_width = obj->tiles_per_width;
    tiles_per_height = obj->tiles_per_height;

    path_depth = obj->path_depth;
    format = obj->format;

    context = obj->context;

    if (Rok4Format::is_raster(format)) {
        nodata_value = new int[channels];
        memcpy ( nodata_value, obj->nodata_value, channels * sizeof(int) );
    } else {
        tables = obj->tables;
    }
}

Level::~Level() {
    if (nodata_value != NULL) delete[] nodata_value;
}


/*
 * A REFAIRE
 */
Image* Level::getbbox ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox< double > bbox, int width, int height, CRS* src_crs, CRS* dst_crs, Interpolation::KernelType interpolation, int& error ) {

    Grid* grid = new Grid ( width, height, bbox );

    grid->bbox.print();

    if ( ! ( grid->reproject ( dst_crs, src_crs ) ) ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Impossible de reprojeter la grid" ;
        error = 1; // BBox invalid
        delete grid;
        return 0;
    }

    //la reprojection peut marcher alors que la bbox contient des NaN
    //cela arrive notamment lors que la bbox envoyée par l'utilisateur n'est pas dans le crs specifié par ce dernier
    if (grid->bbox.xmin != grid->bbox.xmin || grid->bbox.xmax != grid->bbox.xmax || grid->bbox.ymin != grid->bbox.ymin || grid->bbox.ymax != grid->bbox.ymax ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Bbox de la grid contenant des NaN" ;
        error = 1;
        delete grid;
        return 0;
    }

    grid->bbox.print();

    // Calcul de la taille du noyau
    //Maintain previous Lanczos behaviour : Lanczos_2 for resampling and reprojecting
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_2;

    const Kernel& kk = Kernel::get_instance ( interpolation ); // Lanczos_2
    double ratio_x = ( grid->bbox.xmax - grid->bbox.xmin ) / ( tm->get_res() *double ( width ) );
    double ratio_y = ( grid->bbox.ymax - grid->bbox.ymin ) / ( tm->get_res() *double ( height ) );
    double bufx=kk.size ( ratio_x ) + 2.;
    double bufy=kk.size ( ratio_y ) + 2.;

    // bufx<50?bufx=50:0;
    // bufy<50?bufy=50:0; // Pour etre sur de ne pas regresser
    
    BoundingBox<int64_t> bbox_int ( floor ( ( grid->bbox.xmin - tm->get_x0() ) /tm->get_res() - bufx ),
                                    floor ( ( tm->get_y0() - grid->bbox.ymax ) /tm->get_res() - bufy ),
                                    ceil ( ( grid->bbox.xmax - tm->get_x0() ) /tm->get_res() + bufx ),
                                    ceil ( ( tm->get_y0() - grid->bbox.ymin ) /tm->get_res() + bufy ) );

    Image* image = getwindow ( maxTileX, maxTileY, bbox_int, error );
    if ( !image ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Image invalid !"  ;
        return 0;
    }

    image->set_bbox ( BoundingBox<double> ( tm->get_x0() + tm->get_res() * bbox_int.xmin, tm->get_y0() - tm->get_res() * bbox_int.ymax, tm->get_x0() + tm->get_res() * bbox_int.xmax, tm->get_y0() - tm->get_res() * bbox_int.ymin ) );

    grid->affine_transform ( 1./image->get_resx(), -image->get_bbox().xmin/image->get_resx() - 0.5,
                             -1./image->get_resy(), image->get_bbox().ymax/image->get_resy() - 0.5 );

    return new ReprojectedImage ( image, bbox, grid, interpolation );
}


Image* Level::getbbox ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox< double > bbox, int width, int height, Interpolation::KernelType interpolation, int& error ) {

    // On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
    bbox.xmin = ( bbox.xmin - tm->get_x0() ) /tm->get_res();
    bbox.xmax = ( bbox.xmax - tm->get_x0() ) /tm->get_res();
    double tmp = bbox.ymin;
    bbox.ymin = ( tm->get_y0() - bbox.ymax ) /tm->get_res();
    bbox.ymax = ( tm->get_y0() - tmp ) /tm->get_res();

    //A VERIFIER !!!!
    BoundingBox<int64_t> bbox_int ( floor ( bbox.xmin + EPS ),
                                    floor ( bbox.ymin + EPS ),
                                    ceil ( bbox.xmax - EPS ),
                                    ceil ( bbox.ymax - EPS ) );

    if ( bbox_int.xmax - bbox_int.xmin == width && bbox_int.ymax - bbox_int.ymin == height &&
            bbox.xmin - bbox_int.xmin < EPS && bbox_int.xmax - bbox.xmax < EPS &&
            bbox.ymin - bbox_int.ymin < EPS && bbox_int.ymax - bbox.ymax < EPS ) {
        /* L'image demandée est en phase et a les mêmes résolutions que les images du niveau
         *   => pas besoin de réechantillonnage */
        return getwindow ( maxTileX, maxTileY, bbox_int, error );
    }

    // Rappel : les coordonnees de la bbox sont ici en pixels
    double ratio_x = ( bbox.xmax - bbox.xmin ) / width;
    double ratio_y = ( bbox.ymax - bbox.ymin ) / height;

    //Maintain previous Lanczos behaviour : Lanczos_3 for resampling only
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_3;
    const Kernel& kk = Kernel::get_instance ( interpolation ); // Lanczos_3

    // On en prend un peu plus pour ne pas avoir d'effet de bord lors du réechantillonnage
    bbox_int.xmin = floor ( bbox.xmin - kk.size ( ratio_x ) );
    bbox_int.xmax = ceil ( bbox.xmax + kk.size ( ratio_x ) );
    bbox_int.ymin = floor ( bbox.ymin - kk.size ( ratio_y ) );
    bbox_int.ymax = ceil ( bbox.ymax + kk.size ( ratio_y ) );

    Image* imageout = getwindow ( maxTileX, maxTileY, bbox_int, error );
    if ( !imageout ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Image invalid !"  ;
        return 0;
    }

    // On affecte la bonne bbox à l'image source afin que la classe de réechantillonnage calcule les bonnes valeurs d'offset
    if (! imageout->set_dimensions ( bbox_int.xmax - bbox_int.xmin, bbox_int.ymax - bbox_int.ymin, BoundingBox<double> ( bbox_int ), 1.0, 1.0 ) ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Dimensions invalid !"  ;
        return 0;
    }

    return new ResampledImage ( imageout, width, height, ratio_x, ratio_y, bbox, interpolation, false );
}

int euclideanDivisionQuotient ( int64_t i, int n ) {
    int q=i/n;  // Division tronquee
    if ( q<0 ) q-=1;
    if ( q==0 && i<0 ) q=-1;
    return q;
}

int euclideanDivisionRemainder ( int64_t i, int n ) {
    int r=i%n;
    if ( r<0 ) r+=n;
    return r;
}

Image* Level::getwindow ( unsigned int maxTileX, unsigned int maxTileY, BoundingBox< int64_t > bbox, int& error ) { 
    int tile_xmin=euclideanDivisionQuotient ( bbox.xmin,tm->get_tile_width() );
    int tile_xmax=euclideanDivisionQuotient ( bbox.xmax -1,tm->get_tile_width() );
    int nbx = tile_xmax - tile_xmin + 1;
    if ( nbx >= maxTileX ) {
        BOOST_LOG_TRIVIAL(info) <<  "Too Much Tile on X axis"  ;
        error=2;
        return 0;
    }
    if (nbx == 0) {
        BOOST_LOG_TRIVIAL(info) <<  "nbx = 0" ;
        error=1;
        return 0;
    }

    int tile_ymin=euclideanDivisionQuotient ( bbox.ymin,tm->get_tile_height() );
    int tile_ymax = euclideanDivisionQuotient ( bbox.ymax-1,tm->get_tile_height() );
    int nby = tile_ymax - tile_ymin + 1;
    if ( nby >= maxTileY ) {
        BOOST_LOG_TRIVIAL(info) <<  "Too Much Tile on Y axis"  ;
        error=2;
        return 0;
    }
    if (nby == 0) {
        BOOST_LOG_TRIVIAL(info) <<  "nby = 0" ;
        error=1;
        return 0;
    }

    int left[nbx];
    memset ( left,   0, nbx*sizeof ( int ) );
    left[0]=euclideanDivisionRemainder ( bbox.xmin,tm->get_tile_width() );
    int top[nby];
    memset ( top,    0, nby*sizeof ( int ) );
    top[0]=euclideanDivisionRemainder ( bbox.ymin,tm->get_tile_height() );
    int right[nbx];
    memset ( right,  0, nbx*sizeof ( int ) );
    right[nbx - 1] = tm->get_tile_width() - euclideanDivisionRemainder ( bbox.xmax -1,tm->get_tile_width() ) -1;
    int bottom[nby];
    memset ( bottom, 0, nby*sizeof ( int ) );
    bottom[nby- 1] = tm->get_tile_height() - euclideanDivisionRemainder ( bbox.ymax -1,tm->get_tile_height() ) - 1;

    std::vector<std::vector<Image*> > T ( nby, std::vector<Image*> ( nbx ) );
    for ( int y = 0; y < nby; y++ ) {
        for ( int x = 0; x < nbx; x++ ) {
            T[y][x] = get_tile ( tile_xmin + x, tile_ymin + y, left[x], top[y], right[x], bottom[y] );
        }
    }

    if ( nbx == 1 && nby == 1 ) return T[0][0];
    else return new CompoundImage ( T );
}


/*
 * Recuperation du nom de la dalle du cache en fonction de son indice
 */
std::string Level::get_path ( int tilex, int tiley) {
    // Cas normalement filtré en amont (exception WMS/WMTS)
    if ( tilex < 0 || tiley < 0 ) {
        BOOST_LOG_TRIVIAL(error) << "Negative tile indices: " << tilex << "," << tiley ;
        return "";
    }

    int x,y;

    x = tilex / tiles_per_width;
    y = tiley / tiles_per_height;

    return context->get_path(racine,x,y,path_depth);

}


/*
 * @return la tuile d'indice (x,y) du niveau
 */
DataSource* Level::get_encoded_tile ( int x, int y ) { // TODO: return 0 sur des cas d'erreur..
    
    //on stocke une dalle
    // Index de la tuile (cf. ordre de rangement des tuiles)
    int n = ( y % tiles_per_height ) * tiles_per_width + ( x % tiles_per_width );
    std::string path = get_path ( x, y);
    if (path == "") {
        return NULL;
    }
    BOOST_LOG_TRIVIAL(debug) << path;
    return new StoreDataSource ( n, tiles_per_width * tiles_per_height, path, context, Rok4Format::to_mime_type ( format ), Rok4Format::to_encoding( format ) );
}

DataSource* Level::get_decoded_tile ( int x, int y ) {

    DataSource* encoded_data = get_encoded_tile ( x, y );
    if (encoded_data == NULL) return 0;

    size_t size;
    if (encoded_data->get_data ( size ) == NULL) {
        delete encoded_data;
        return 0;
    }

    if ( format==Rok4Format::TIFF_RAW_UINT8 || format==Rok4Format::TIFF_RAW_FLOAT32 )
        return encoded_data;
    else if ( format==Rok4Format::TIFF_JPG_UINT8 || format==Rok4Format::TIFF_JPG90_UINT8 )
        return new DataSourceDecoder<JpegDecoder> ( encoded_data );
    else if ( format==Rok4Format::TIFF_PNG_UINT8 )
        return new DataSourceDecoder<PngDecoder> ( encoded_data );
    else if ( format==Rok4Format::TIFF_LZW_UINT8 || format == Rok4Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encoded_data );
    else if ( format==Rok4Format::TIFF_ZIP_UINT8 || format == Rok4Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encoded_data );
    else if ( format==Rok4Format::TIFF_PKB_UINT8 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        return new DataSourceDecoder<PackBitsDecoder> ( encoded_data );
    BOOST_LOG_TRIVIAL(error) <<  "Type d'encodage inconnu : " <<format  ;
    return 0;
}


DataSource* Level::get_tile (int x, int y) {

    DataSource* source = get_encoded_tile ( x, y );
    if (source == NULL) return NULL;

    size_t size;
    if (source->get_data ( size ) == NULL) {
        delete source;
        return NULL;
    }

    if ( format == Rok4Format::TIFF_RAW_UINT8 || format == Rok4Format::TIFF_LZW_UINT8 ||
         format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_UINT8 ||
         format == Rok4Format::TIFF_PKB_FLOAT32 || format == Rok4Format::TIFF_PKB_UINT8
        )
    {
        BOOST_LOG_TRIVIAL(debug) <<  "GetTile Tiff"  ;
        TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource ( source,format,channels,tm->get_tile_width(), tm->get_tile_height() );
        return fullTiffDS;
    }

    return source;
}

Image* Level::get_tile ( int x, int y, int left, int top, int right, int bottom ) {
    int pixel_size=1;
    BOOST_LOG_TRIVIAL(debug) <<  "GetTile Image"  ;
    if ( format==Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;

    DataSource* ds = get_decoded_tile ( x,y );

    BoundingBox<double> bb ( 
        tm->get_x0() + x * tm->get_tile_width() * tm->get_res() + left * tm->get_res(),
        tm->get_y0() - ( y+1 ) * tm->get_tile_height() * tm->get_res() + bottom * tm->get_res(),
        tm->get_x0() + ( x+1 ) * tm->get_tile_width() * tm->get_res() - right * tm->get_res(),
        tm->get_y0() - y * tm->get_tile_height() * tm->get_res() - top * tm->get_res()
    );

    if (ds == 0) {
        // On crée une image monochrome (valeur fournie dans la pyramide) de la taille qu'aurait du avoir la tuile demandée
        EmptyImage* ei = new EmptyImage(
            tm->get_tile_width() - left - right, // width
            tm->get_tile_height() - top - bottom, // height
            channels,
            nodata_value
        );
        ei->set_bbox(bb);
        return ei;
    } else {
        return new ImageDecoder (
            ds, tm->get_tile_width(), tm->get_tile_height(), channels, bb,
            left, top, right, bottom, pixel_size
        );
    }
}

TileMatrix* Level::get_tm () { return tm; }
Rok4Format::eFormat Level::get_format () { return format; }
int Level::get_channels () { return channels; }
TileMatrixLimits Level::get_tile_limits () { return tm_limits; }

uint32_t Level::get_max_tile_row() {
    return tm_limits.max_tile_row;
}
uint32_t Level::get_min_tile_row() {
    return tm_limits.min_tile_row;
}
uint32_t Level::get_max_tile_col() {
    return tm_limits.max_tile_col;
}
uint32_t Level::get_min_tile_col() {
    return tm_limits.min_tile_col;
}
BoundingBox<double> Level::get_bbox_from_tile_limits() {
    return tm->bbox_from_tile_limits(tm_limits);
}
void Level::set_tile_limits_from_bbox(BoundingBox<double> bb) {
    tm_limits = tm->bbox_to_tile_limits(bb);
}

double Level::get_res () { return tm->get_res(); }
std::string Level::get_id () { return tm->get_id(); }
uint32_t Level::get_tiles_per_width () { return tiles_per_width; }
uint32_t Level::get_tiles_per_height () { return tiles_per_height; }
Context* Level::get_context () { return context; }
std::vector<Table>* Level::get_tables() { return &tables; }
