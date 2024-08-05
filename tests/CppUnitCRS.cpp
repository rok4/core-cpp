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

#include <string>
#include "rok4/utils/CRS.h"

class CppUnitCRS : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitCRS );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

protected:
    std::string crs_code1;
    std::string crs_code2;
    std::string crs_code3;
    std::string crs_code4;
    std::string crs_code5;
    std::string crs_code6;

    CRS* crs1;
    CRS* crs2;
    CRS* crs3;
    CRS* crs4;
    CRS* crs5;
    CRS* crs6;

    double bbox1xmin;
    double bbox1ymin;
    double bbox1xmax;
    double bbox1ymax;

public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitCRS );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitCRS, "CppUnitCRS" );

void CppUnitCRS::setUp() {
    crs_code1="EPSG:4326";
    crs_code2="CRS:84";
    crs_code3="EPSG:3857";
    crs_code4="IGNF:WGS84G";
    crs_code5="epsg:3857";
    crs_code6="ESPG:3857";

    crs1 = new CRS ( crs_code1 );
    crs2 = new CRS ( crs_code2 );
    crs3 = new CRS ( crs_code3 );
    crs4 = new CRS ( crs_code4 );
    crs5 = new CRS ( crs_code5 );
    crs6 = new CRS ( crs_code6 );

    bbox1xmin = -180;
    bbox1ymin = -90;
    bbox1xmax = 180;
    bbox1ymax = 90;
}

void CppUnitCRS::constructors() {
    CRS crs3cpy ( crs3 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crs3cpy == *crs3 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crs3cpy.cmp_request_code ( crs3->get_request_code() ) );

    CRS crsempty;
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.get_proj_code().compare ( "NO_PROJ_CODE" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.cmp_request_code ( "" ) );
}
void CppUnitCRS::getters() {
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",crs1->is_define() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",crs2->is_define() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",crs3->is_define() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",crs4->is_define() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",crs5->is_define() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj compatible",!crs6->is_define() );

    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs1->is_lon_lat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs2->is_lon_lat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs3->is_lon_lat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs4->is_lon_lat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs5->is_lon_lat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs6->is_lon_lat() );

    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs1->cmp_request_code(crs_code1));
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs2->cmp_request_code(crs_code2));
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs3->cmp_request_code(crs_code3));
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs4->cmp_request_code(crs_code4));
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs5->cmp_request_code(crs_code5));
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_request_code",crs6->cmp_request_code(crs_code6));

    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs1->get_proj_code().compare ( "EPSG:4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs2->get_proj_code().compare ( "EPSG:4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs3->get_proj_code().compare ( "EPSG:3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs4->get_proj_code().compare ( "IGNF:WGS84G" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs5->get_proj_code().compare ( "EPSG:3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_proj_code",crs6->get_proj_code().compare ( "NO_PROJ_CODE" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs1->get_authority().compare ( "EPSG" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs2->get_authority().compare ( "CRS" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs3->get_authority().compare ( "EPSG" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs4->get_authority().compare ( "IGNF" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs5->get_authority().compare ( "EPSG" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_authority",crs6->get_authority().compare ( "ESPG" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs1->get_identifier().compare ( "4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs2->get_identifier().compare ( "84" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs3->get_identifier().compare ( "3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs4->get_identifier().compare ( "WGS84G" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs5->get_identifier().compare ( "3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS get_identifier",crs6->get_identifier().compare ( "3857" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->get_crs_definition_area().xmin == bbox1xmin );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->get_crs_definition_area().xmax == bbox1xmax );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->get_crs_definition_area().ymin == bbox1ymin );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->get_crs_definition_area().ymax == bbox1ymax );
}

void CppUnitCRS::tearDown() {
    delete crs1;
    delete crs2;
    delete crs3;
    delete crs4;
    delete crs5;
    delete crs6;
}
