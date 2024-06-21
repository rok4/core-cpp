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

#include <cppunit/extensions/HelperMacros.h>

#include "rok4/image/ReprojectedImage.h"
#include "rok4/image/EmptyImage.h"
#include <sys/time.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

class CppUnitReprojectedImage : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitReprojectedImage );
    CPPUNIT_TEST ( performance );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {};

protected:

    string name ( int kernel_type ) {
        switch ( kernel_type ) {
        case Interpolation::UNKNOWN:
            return "(Default) Lanczos 3";
        case Interpolation::NEAREST_NEIGHBOUR:
            return "Nearest Neighbour";
        case Interpolation::LINEAR:
            return "Linear";
        case Interpolation::CUBIC:
            return "Cubic";
        case Interpolation::LANCZOS_2:
            return "Lanczos 2";
        case Interpolation::LANCZOS_3:
            return "Lanczos 3";
        case Interpolation::LANCZOS_4:
            return "Lanczos 4";
        }

        return "(Default) Lanczos 3";
    }

    void _chrono ( int channels, int kernel_type ) {
        int color[4];
        float buffer[800*4] __attribute__ ( ( aligned ( 32 ) ) );
        int nb_iteration = 2;

        timeval BEGIN, NOW;
        gettimeofday ( &BEGIN, NULL );

        for ( int i = 0; i < nb_iteration; i++ ) {
            BoundingBox<double> bbox_dst ( 500000, 6500000, 501000, 6501000 );
            bbox_dst.crs = "EPSG:4326";
            Grid* grid = new Grid ( 800, 600, bbox_dst );

            CRS* crs_src = new CRS ( "EPSG:4326" );
            CRS* crs_dst = new CRS ( "IGNF:LAMB93" );

            grid->reproject ( crs_dst, crs_src );

            Image* image = new EmptyImage ( 1024, 768, channels, color );
            BoundingBox<double> bbox_src ( grid->bbox.xmin - 100, grid->bbox.ymin - 100, grid->bbox.xmax + 100, grid->bbox.ymax + 100 );
            bbox_src.crs = "IGNF:LAMB93";
            image->set_bbox (bbox_src);
            image->set_crs(crs_src);

            grid->affine_transform ( 1./image->get_resx(), -image->get_bbox().xmin/image->get_resx() - 0.5,
                                     -1./image->get_resy(), image->get_bbox().ymax/image->get_resy() - 0.5 );

            ReprojectedImage* R = new ReprojectedImage ( image,  bbox_dst, grid, Interpolation::KernelType ( kernel_type ) );
            for ( int l = 0; l < 600; l++ ) R->get_line ( buffer, l );
            delete R;
            delete crs_src;
            delete crs_dst;
        }
        gettimeofday ( &NOW, NULL );
        double time = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        cerr << time << "s : " << nb_iteration << " reprojection 800x600, " << channels << " canaux, " << name ( kernel_type ) << endl;
    }

    void performance() {
        for ( int i = 1; i <= 4; i++ ) {
            for ( int j = 1; j < 7; j++ )
                _chrono ( i, j );
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitReprojectedImage );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitReprojectedImage, "CppUnitReprojectedImage" );

