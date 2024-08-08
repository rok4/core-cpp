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
 * \file CRS.cpp
 * \~french
 * \brief Implémente de la gestion des systèmes de référence
 * \~english
 * \brief implement the reference systems handler
 */

#include "utils/CRS.h"
#include <boost/log/trivial.hpp>
#include <proj.h>
#include "utils/Cache.h"

CRS CRS::epsg4326;

/**
 * \~french \brief Transforme la chaîne fournie en minuscule
 * \~english \brief Transform the string to lowercase
 */
std::string to_lower_case ( std::string str ) {
    std::string lc_str=str;
    for ( int i = 0; str[i]; i++ ) lc_str[i] = tolower ( str[i] );
    return lc_str;
}

/**
 * \~french \brief Transforme la chaîne fournie en majuscule
 * \~english \brief Transform the string to uppercase
 */
std::string to_upper_case ( std::string str ) {
    std::string uc_str=str;
    for ( int i = 0; str[i]; i++ ) uc_str[i] = toupper ( str[i] );
    return uc_str;
}

CRS::CRS() : definition_area ( -90.0,-180.0,90.0,180.0 ), native_definition_area ( 0,0,0,0 ) {
    definition_area.crs = "EPSG:4326";
    request_code = "";
    proj_code = NO_PROJ_CODE;
    pj_proj = 0;
}

CRS::CRS ( std::string crs_code ) : definition_area ( -90.0,-180.0,90.0,180.0 ), native_definition_area ( 0,0,0,0 ) {
    definition_area.crs = "EPSG:4326";
    request_code=to_upper_case(crs_code);
    proj_code=to_upper_case(crs_code);
    pj_proj = 0;

    if ( request_code == "CRS:84" ) proj_code = "EPSG:4326";

    PJ_CONTEXT* pj_ctx = ProjPool::get_proj_env();
    pj_proj = proj_create ( pj_ctx, proj_code.c_str());

    if ( 0 == pj_proj ) {
        proj_code = NO_PROJ_CODE;
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ avec " << request_code << " : " << proj_errno_string ( err )  ;
        return;
    }

    proj_get_area_of_use(pj_ctx, pj_proj, & ( definition_area.xmin ), & ( definition_area.ymin ), & ( definition_area.xmax ), & ( definition_area.ymax ), NULL);

    native_definition_area = definition_area;

    if (proj_code != "EPSG:4326") {
        native_definition_area.reproject(CRS::get_epsg4326(), this);
    }
}

CRS::CRS ( CRS* crs ) : definition_area ( crs->definition_area ), native_definition_area ( crs->native_definition_area ) {
    request_code=crs->request_code;
    proj_code=crs->proj_code;
    PJ_CONTEXT* pj_ctx = ProjPool::get_proj_env();
    pj_proj = proj_create ( pj_ctx, proj_code.c_str());
}


CRS& CRS::operator= ( const CRS& other ) {
    if ( this != &other ) {
        this->proj_code = other.proj_code;
        this->request_code = other.request_code;
        this->definition_area = other.definition_area;
        this->native_definition_area = other.native_definition_area;

        if (this->pj_proj != 0) {
            proj_destroy (pj_proj);
        }

        PJ_CONTEXT* pj_ctx = ProjPool::get_proj_env();
        this->pj_proj = proj_create ( pj_ctx, this->proj_code.c_str());
    }
    return *this;
}

bool CRS::is_lon_lat() {

    if (pj_proj == 0) return false;

    PJ_TYPE type = proj_get_type ( pj_proj );
    
    return (type == PJ_TYPE_GEOGRAPHIC_2D_CRS || type ==  PJ_TYPE_GEOGRAPHIC_3D_CRS);
}


long double CRS::get_meters_per_unit() {
    // Hypothese : un CRS en long/lat est en degres
    // R=6378137m
    if ( is_lon_lat() )
        return 111319.49079327357264771338267056;
    else
        return 1.0;
}

bool CRS::cmp_request_code ( std::string crs ) {
    return request_code == to_upper_case ( crs );
}

std::string CRS::get_authority() {
    size_t pos=request_code.find ( ':' );
    if ( pos<1 || pos >=request_code.length() ) {
        BOOST_LOG_TRIVIAL(error) <<   "Erreur sur le CRS " << request_code << " : absence de separateur"  ;
        pos = request_code.length();
    }
    return ( request_code.substr ( 0,pos ) );
}

std::string CRS::get_identifier() {
    size_t pos=request_code.find ( ':' );
    if ( pos<1 || pos >=request_code.length() ) {
        BOOST_LOG_TRIVIAL(error) <<   "Erreur sur le CRS " << request_code << " : absence de separateur"  ;
        pos = -1;
    }
    return ( request_code.substr ( pos+1 ) );
}

bool CRS::operator== ( const CRS& crs ) const {
    return ( proj_code == crs.proj_code );
}

bool CRS::operator!= ( const CRS& crs ) const {
    return ! ( *this == crs );
}

std::string CRS::get_proj_param ( std::string paramName ) {
    std::size_t pos = 0, find = 1, find_equal = 0;
    PJ_CONTEXT* pj_ctx = ProjPool::get_proj_env();
    std::string def( proj_as_proj_string(pj_ctx, pj_proj, PJ_PROJ_4, NULL) );

    pos = to_lower_case( def ).find( "+" + to_lower_case( paramName ) + "=" );
    if ( pos <0 || pos >def.size() ) {
        return "";
    }
    find_equal = to_lower_case( def ).find( "=", pos );
    find = to_lower_case( def ).find( " ", pos );
    //BOOST_LOG_TRIVIAL(debug) <<  "Valeur du paramètre " + paramName + " : [" + to_lower_case( def ).substr(find_equal+1, find - find_equal -1) + "]"  ;
    return to_lower_case( def ).substr(find_equal+1, find - find_equal -1);
}

bool CRS::test_proj_param ( std::string paramName ) {
    std::size_t pos = 0;
    PJ_CONTEXT* pj_ctx = ProjPool::get_proj_env();
    std::string def( proj_as_proj_string(pj_ctx, pj_proj, PJ_PROJ_4, NULL) );
    pos = to_lower_case( def ).find( "+" + to_lower_case( paramName ));
    if ( pos <0 || pos >def.size() ) {
        return false;
    }
    return true;
}

CRS::~CRS() {
    if (pj_proj != 0) {
        proj_destroy (pj_proj);
        pj_proj = 0;
    }
}

CRS* CRS::get_epsg4326() {
    
    if (epsg4326.proj_code == NO_PROJ_CODE) {
        // On instancie le CRS de classe 4326
        epsg4326 = CRS ( "EPSG:4326");
    }

    return &(epsg4326);
}
