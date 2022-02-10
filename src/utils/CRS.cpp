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
std::string toLowerCase ( std::string str ) {
    std::string lc_str=str;
    for ( int i = 0; str[i]; i++ ) lc_str[i] = tolower ( str[i] );
    return lc_str;
}

/**
 * \~french \brief Transforme la chaîne fournie en majuscule
 * \~english \brief Transform the string to uppercase
 */
std::string toUpperCase ( std::string str ) {
    std::string uc_str=str;
    for ( int i = 0; str[i]; i++ ) uc_str[i] = toupper ( str[i] );
    return uc_str;
}

CRS::CRS() : definitionArea ( -90.0,-180.0,90.0,180.0 ), nativeDefinitionArea ( 0,0,0,0 ) {
    definitionArea.crs = "EPSG:4326";
    requestCode = "";
    projCode = NO_PROJ_CODE;
    pj_proj = 0;
}

CRS::CRS ( std::string crs_code ) : definitionArea ( -90.0,-180.0,90.0,180.0 ), nativeDefinitionArea ( 0,0,0,0 ) {
    definitionArea.crs = "EPSG:4326";
    requestCode=toUpperCase(crs_code);
    projCode=toUpperCase(crs_code);
    pj_proj = 0;

    if ( requestCode == "CRS:84" ) projCode = "EPSG:4326";

    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();
    pj_proj = proj_create ( pj_ctx, projCode.c_str());

    if ( 0 == pj_proj ) {
        projCode = NO_PROJ_CODE;
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ avec " << requestCode << " : " << proj_errno_string ( err )  ;
        return;
    }

    proj_get_area_of_use(pj_ctx, pj_proj, & ( definitionArea.xmin ), & ( definitionArea.ymin ), & ( definitionArea.xmax ), & ( definitionArea.ymax ), NULL);

    nativeDefinitionArea = definitionArea;

    if (projCode != "EPSG:4326") {
        nativeDefinitionArea.reproject(CRS::getEpsg4326(), this);
    }
}

CRS::CRS ( CRS* crs ) : definitionArea ( crs->definitionArea ), nativeDefinitionArea ( crs->nativeDefinitionArea ) {
    requestCode=crs->requestCode;
    projCode=crs->projCode;
    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();
    pj_proj = proj_create ( pj_ctx, projCode.c_str());
}


CRS& CRS::operator= ( const CRS& other ) {
    if ( this != &other ) {
        this->projCode = other.projCode;
        this->requestCode = other.requestCode;
        this->definitionArea = other.definitionArea;
        this->nativeDefinitionArea = other.nativeDefinitionArea;

        if (this->pj_proj != 0) {
            proj_destroy (pj_proj);
        }

        PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();
        this->pj_proj = proj_create ( pj_ctx, this->projCode.c_str());
    }
    return *this;
}

bool CRS::isLongLat() {

    if (pj_proj == 0) return false;

    PJ_TYPE type = proj_get_type ( pj_proj );
    
    return (type == PJ_TYPE_GEOGRAPHIC_2D_CRS || type ==  PJ_TYPE_GEOGRAPHIC_3D_CRS);
}


long double CRS::getMetersPerUnit() {
    // Hypothese : un CRS en long/lat est en degres
    // R=6378137m
    if ( isLongLat() )
        return 111319.49079327357264771338267056;
    else
        return 1.0;
}

bool CRS::cmpRequestCode ( std::string crs ) {
    return requestCode == toUpperCase ( crs );
}

std::string CRS::getAuthority() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        BOOST_LOG_TRIVIAL(error) <<   "Erreur sur le CRS " << requestCode << " : absence de separateur"  ;
        pos = requestCode.length();
    }
    return ( requestCode.substr ( 0,pos ) );
}

std::string CRS::getIdentifier() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        BOOST_LOG_TRIVIAL(error) <<   "Erreur sur le CRS " << requestCode << " : absence de separateur"  ;
        pos = -1;
    }
    return ( requestCode.substr ( pos+1 ) );
}

bool CRS::operator== ( const CRS& crs ) const {
    return ( projCode == crs.projCode );
}

bool CRS::operator!= ( const CRS& crs ) const {
    return ! ( *this == crs );
}

std::string CRS::getProjParam ( std::string paramName ) {
    std::size_t pos = 0, find = 1, find_equal = 0;
    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();
    std::string def( proj_as_proj_string(pj_ctx, pj_proj, PJ_PROJ_4, NULL) );

    pos = toLowerCase( def ).find( "+" + toLowerCase( paramName ) + "=" );
    if ( pos <0 || pos >def.size() ) {
        return "";
    }
    find_equal = toLowerCase( def ).find( "=", pos );
    find = toLowerCase( def ).find( " ", pos );
    //BOOST_LOG_TRIVIAL(debug) <<  "Valeur du paramètre " + paramName + " : [" + toLowerCase( def ).substr(find_equal+1, find - find_equal -1) + "]"  ;
    return toLowerCase( def ).substr(find_equal+1, find - find_equal -1);
}

bool CRS::testProjParam ( std::string paramName ) {
    std::size_t pos = 0;
    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();
    std::string def( proj_as_proj_string(pj_ctx, pj_proj, PJ_PROJ_4, NULL) );
    pos = toLowerCase( def ).find( "+" + toLowerCase( paramName ));
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

CRS* CRS::getEpsg4326() {
    
    if (epsg4326.projCode == NO_PROJ_CODE) {
        // On instancie le CRS de classe 4326
        epsg4326 = CRS ( "EPSG:4326");
    }

    return &(epsg4326);
}
