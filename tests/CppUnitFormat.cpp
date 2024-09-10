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

#include <string.h>
#include "rok4/enums/Format.h"

class CppUnitFormat : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitFormat );
    CPPUNIT_TEST ( testFormat );
    CPPUNIT_TEST ( formatFromString );
    CPPUNIT_TEST ( formatToMime );
    CPPUNIT_TEST_SUITE_END();

public:
    void testFormat();
    void formatFromString();
    void formatToMime();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitFormat );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitFormat, "CppUnitFormat" );

void CppUnitFormat::testFormat() {
    Rok4Format::eFormat ukn = Rok4Format::UNKNOWN;
    Rok4Format::eFormat tri8 = Rok4Format::TIFF_RAW_UINT8;
    Rok4Format::eFormat tji8 = Rok4Format::TIFF_JPG_UINT8;
    Rok4Format::eFormat tpi8 = Rok4Format::TIFF_PNG_UINT8;
    Rok4Format::eFormat tli8 = Rok4Format::TIFF_LZW_UINT8;
    Rok4Format::eFormat trf32 = Rok4Format::TIFF_RAW_FLOAT32;
    Rok4Format::eFormat tlf32 = Rok4Format::TIFF_LZW_FLOAT32;
    Rok4Format::eFormat tzi8 = Rok4Format::TIFF_ZIP_UINT8;
    Rok4Format::eFormat tzf32 = Rok4Format::TIFF_ZIP_FLOAT32;

    CPPUNIT_ASSERT_MESSAGE ( "Unknown is false", ! ( ukn ) );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_UINT8", Rok4Format::to_string ( tri8 ).compare ( "TIFF_RAW_UINT8" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_JPG_UINT8", Rok4Format::to_string ( tji8 ).compare ( "TIFF_JPG_UINT8" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_PNG_UINT8", Rok4Format::to_string ( tpi8 ).compare ( "TIFF_PNG_UINT8" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_UINT8", Rok4Format::to_string ( tli8 ).compare ( "TIFF_LZW_UINT8" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_FLOAT32", Rok4Format::to_string ( trf32 ).compare ( "TIFF_RAW_FLOAT32" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_FLOAT32", Rok4Format::to_string ( tlf32 ).compare ( "TIFF_LZW_FLOAT32" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_UINT8", Rok4Format::to_string ( tzi8 ).compare ( "TIFF_ZIP_UINT8" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_FLOAT32", Rok4Format::to_string ( tzf32 ).compare ( "TIFF_ZIP_FLOAT32" ) == 0 );
}

void CppUnitFormat::formatFromString() {
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_UINT8", Rok4Format::from_string ( "TIFF_RAW_UINT8" ) == Rok4Format::TIFF_RAW_UINT8 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_JPG_UINT8", Rok4Format::from_string ( "TIFF_JPG_UINT8" ) == Rok4Format::TIFF_JPG_UINT8 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_PNG_UINT8", Rok4Format::from_string ( "TIFF_PNG_UINT8" ) == Rok4Format::TIFF_PNG_UINT8 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_UINT8", Rok4Format::from_string ( "TIFF_LZW_UINT8" ) == Rok4Format::TIFF_LZW_UINT8 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_FLOAT32", Rok4Format::from_string ( "TIFF_RAW_FLOAT32" ) == Rok4Format::TIFF_RAW_FLOAT32 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_FLOAT32", Rok4Format::from_string ( "TIFF_LZW_FLOAT32" ) == Rok4Format::TIFF_LZW_FLOAT32 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_FLOAT32", Rok4Format::from_string ( "TIFF_ZIP_FLOAT32" ) == Rok4Format::TIFF_ZIP_FLOAT32 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_UINT8", Rok4Format::from_string ( "TIFF_ZIP_UINT8" ) == Rok4Format::TIFF_ZIP_UINT8 );
    CPPUNIT_ASSERT_MESSAGE ( "Wrong Value", Rok4Format::from_string ( "Wrong" ) == Rok4Format::UNKNOWN );
}

void CppUnitFormat::formatToMime() {
    Rok4Format::eFormat ukn = Rok4Format::UNKNOWN;
    Rok4Format::eFormat tri8 = Rok4Format::TIFF_RAW_UINT8;
    Rok4Format::eFormat tji8 = Rok4Format::TIFF_JPG_UINT8;
    Rok4Format::eFormat tpi8 = Rok4Format::TIFF_PNG_UINT8;
    Rok4Format::eFormat tli8 = Rok4Format::TIFF_LZW_UINT8;
    Rok4Format::eFormat trf32 = Rok4Format::TIFF_RAW_FLOAT32;
    Rok4Format::eFormat tlf32 = Rok4Format::TIFF_LZW_FLOAT32;
    Rok4Format::eFormat tzi8 = Rok4Format::TIFF_ZIP_UINT8;
    Rok4Format::eFormat tzf32 = Rok4Format::TIFF_ZIP_FLOAT32;

    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_UINT8", Rok4Format::to_mime_type ( tri8 ).compare ( "image/tiff" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_JPG_UINT8", Rok4Format::to_mime_type ( tji8 ).compare ( "image/jpeg" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_PNG_UINT8", Rok4Format::to_mime_type ( tpi8 ).compare ( "image/png" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_UINT8", Rok4Format::to_mime_type ( tli8 ).compare ( "image/tiff" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_RAW_FLOAT32", Rok4Format::to_mime_type ( trf32 ).compare ( "image/x-bil;bits=32" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_LZW_FLOAT32", Rok4Format::to_mime_type ( tlf32 ).compare ( "image/tiff" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_UINT8", Rok4Format::to_mime_type ( tzi8 ).compare ( "image/tiff" ) == 0 );
    CPPUNIT_ASSERT_MESSAGE ( "TIFF_ZIP_FLOAT32", Rok4Format::to_mime_type ( tzf32 ).compare ( "image/x-bil;bits=32" ) == 0 );
}
