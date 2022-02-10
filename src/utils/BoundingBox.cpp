/*
 * Copyright © (2011) Institut national de l'information
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
 * \file BoundingBox.h
 ** \~french
 * \brief Implémentation de la classe template BoundingBox
 ** \~english
 * \brief Implement template class BoundingBox
 */

#include "utils/BoundingBox.h"
#include "utils/CRS.h"

template<typename T>
bool BoundingBox<T>::isInAreaOfCRS(CRS* c) {

    if (crs == "EPSG:4326") {
        return c->getCrsDefinitionArea().contains(*this);
    } else if (c->cmpRequestCode(crs)) {
        return c->getNativeCrsDefinitionArea().contains(*this);
    }

    BOOST_LOG_TRIVIAL(warning) <<  "TODO : isInAreaOfCRS pour une bbox ni géographique ni dans le CRS fourni : " << crs ;
    return false;
}

template<typename T>
bool BoundingBox<T>::intersectAreaOfCRS(CRS* c) {

    if (crs == "EPSG:4326") {
        return c->getCrsDefinitionArea().intersects(*this);
    } else if (c->cmpRequestCode(crs)) {
        return c->getNativeCrsDefinitionArea().intersects(*this);
    }

    BOOST_LOG_TRIVIAL(warning) <<  "TODO : intersectAreaOfCRS pour une bbox non géographique ni dans le CRS fourni : " << crs ;
    return false;
}

template<typename T>
BoundingBox<T> BoundingBox<T>::cropToAreaOfCRS ( CRS* c ) {

    if (crs == "EPSG:4326") {
        return c->getCrsDefinitionArea().getIntersection(*this);
    } else if (c->cmpRequestCode(crs)) {
        return c->getNativeCrsDefinitionArea().getIntersection(*this);
    }

    BOOST_LOG_TRIVIAL(warning) <<  "TODO : cropToAreaOfCRS pour une bbox non géographique ni dans le CRS fourni" ;
    return BoundingBox<T> (0,0,0,0);
}

template<typename T>
bool BoundingBox<T>::reproject ( CRS* from_crs, CRS* to_crs , int nbSegment ) {

    PJ_CONTEXT* pj_ctx = ProjPool::getProjEnv();

    PJ *pj_conv_raw, *pj_conv_normalize;
    pj_conv_raw = proj_create_crs_to_crs_from_pj ( pj_ctx, from_crs->getProjObject(), to_crs->getProjObject(), NULL, NULL);
    if (0 == pj_conv_raw) {
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ pour la reprojection de la bbox (création) " << from_crs->getRequestCode() << " -> " << to_crs->getRequestCode() << " : " << proj_errno_string ( err )  ;
        return false;
    }

    pj_conv_normalize = proj_normalize_for_visualization(pj_ctx, pj_conv_raw);
    proj_destroy (pj_conv_raw);
    if (0 == pj_conv_normalize) {
        int err = proj_context_errno ( pj_ctx );
        BOOST_LOG_TRIVIAL(error) <<   "Erreur PROJ pour la reprojection de la bbox (normalisation) " << from_crs->getRequestCode() << " -> " << to_crs->getRequestCode() << " : " << proj_errno_string ( err )  ;
        return false;
    }

    T stepX = ( xmax - xmin ) / T ( nbSegment );
    T stepY = ( ymax - ymin ) / T ( nbSegment );

    PJ_COORD points[nbSegment*4];

    for ( int i = 0; i < nbSegment; i++ ) {
        points[4*i] = proj_coord(xmin + i*stepX, ymin, 0, 0);
        points[4*i + 1] = proj_coord(xmin + i*stepX, ymax, 0, 0);
        points[4*i + 2] = proj_coord(xmin, ymin + i*stepY, 0, 0);
        points[4*i + 3] = proj_coord(xmax, ymin + i*stepY, 0, 0);
    }

    int code = proj_trans_array ( pj_conv_normalize, PJ_FWD, nbSegment*4, points );

    if ( code != 0 ) {
        BOOST_LOG_TRIVIAL(error) <<   "Code erreur proj : " << proj_errno_string(code)  ;
        return false;
    }

    xmin = points[0].xy.x;
    xmax = points[0].xy.x;
    ymin = points[0].xy.y;
    ymax = points[0].xy.y;

    for ( int i = 1; i < nbSegment*4; i++ ) {
        xmin = std::min ( xmin, points[i].xy.x );
        xmax = std::max ( xmax, points[i].xy.x );
        ymin = std::min ( ymin, points[i].xy.y );
        ymax = std::max ( ymax, points[i].xy.y );
    }

    proj_destroy (pj_conv_normalize);

    crs = to_crs->getRequestCode();

    return true;
}

template class BoundingBox<double>;