if(DEPENDENCIES_FOUND)
    return()
endif(DEPENDENCIES_FOUND)

message("Search dependencies...")

# Extern libraries

# Dynamique

if(NOT TARGET boostlog)
    find_package(BoostLog)
    if(BOOSTLOG_FOUND)
        add_library(boostlog SHARED IMPORTED)
        set_property(TARGET boostlog PROPERTY IMPORTED_LOCATION ${BOOSTLOG_LIBRARY})
        add_library(boostlogsetup SHARED IMPORTED)
        set_property(TARGET boostlogsetup PROPERTY IMPORTED_LOCATION ${BOOSTLOGSETUP_LIBRARY})
        add_library(boostthread SHARED IMPORTED)
        set_property(TARGET boostthread PROPERTY IMPORTED_LOCATION ${BOOSTTHREAD_LIBRARY})
        add_library(boostsystem SHARED IMPORTED)
        set_property(TARGET boostsystem PROPERTY IMPORTED_LOCATION ${BOOSTSYSTEM_LIBRARY})
        add_library(boostfilesystem SHARED IMPORTED)
        set_property(TARGET boostfilesystem PROPERTY IMPORTED_LOCATION ${BOOSTFILESYSTEM_LIBRARY})
        add_definitions(-DBOOST_LOG_DYN_LINK -DBOOST_SYSTEM_USE_UTF8)
    else(BOOSTLOG_FOUND)
        message(FATAL_ERROR "Cannot find extern library boostlog")
    endif(BOOSTLOG_FOUND)
endif(NOT TARGET boostlog)

if(NOT TARGET zlib)
    find_package(Zlib)
    if(ZLIB_FOUND)
        add_library(zlib SHARED IMPORTED)
        set_property(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY})
    else(ZLIB_FOUND)
        message(FATAL_ERROR "Cannot find extern library zlib")
    endif(ZLIB_FOUND)
endif(NOT TARGET zlib)


if(NOT TARGET tiff)
    find_package(Tiff)
    if(TIFF_FOUND)
        add_library(tiff SHARED IMPORTED)
        set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${TIFF_LIBRARY})
    else(TIFF_FOUND)
        message(FATAL_ERROR "Cannot find extern library libtiff")
    endif(TIFF_FOUND)
endif(NOT TARGET tiff)

if(NOT TARGET png)
    find_package(PNG)
    if(PNG_FOUND)
        add_library(png SHARED IMPORTED)
        set_property(TARGET png PROPERTY IMPORTED_LOCATION ${PNG_LIBRARY})
    else(PNG_FOUND)
        if(BUILD_DEPENDENCIES)
            message(FATAL_ERROR "Cannot find extern library libpng")
        endif(BUILD_DEPENDENCIES)  
    endif(PNG_FOUND)
endif(NOT TARGET png)

IF(KDU_ENABLED)
    message( FATAL_ERROR "Use Kakadu driver to read JPEG2000 is no more available" )
ELSE(KDU_ENABLED)
    if(NOT TARGET jpeg2000)
        find_package(Openjpeg)
        if(OPENJPEG_FOUND)
            add_library(jpeg2000 SHARED IMPORTED)
            set_property(TARGET jpeg2000 PROPERTY IMPORTED_LOCATION ${OPENJPEG_LIBRARY})
        else(OPENJPEG_FOUND)
            message(FATAL_ERROR "Cannot find extern library libopenjpeg")
        endif(OPENJPEG_FOUND)
    endif(NOT TARGET jpeg2000)
ENDIF(KDU_ENABLED)
    

if(NOT TARGET curl)
    find_package(Curl)
    if(CURL_FOUND)
        add_library(curl SHARED IMPORTED)
        set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CURL_LIBRARY})
    else(CURL_FOUND)
        message(FATAL_ERROR "Cannot find extern library libcurl")
    endif(CURL_FOUND)
endif(NOT TARGET curl)

if(NOT TARGET openssl)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        add_library(openssl SHARED IMPORTED)
        set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${OPENSSL_LIBRARY})
        add_library(crypto SHARED IMPORTED)
        set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${CRYPTO_LIBRARY})
    else(OPENSSL_FOUND)
        message(FATAL_ERROR "Cannot find extern library openssl and crypto")
    endif(OPENSSL_FOUND)
endif(NOT TARGET openssl)

if(NOT TARGET turbojpeg)
    find_package(TurboJpeg)
    if(TURBOJPEG_FOUND)
        add_library(turbojpeg SHARED IMPORTED)
        set_property(TARGET turbojpeg PROPERTY IMPORTED_LOCATION ${TURBOJPEG_LIBRARY})
    else(TURBOJPEG_FOUND)
        message(FATAL_ERROR "Cannot find extern library libturbojpeg")
    endif(TURBOJPEG_FOUND)
endif(NOT TARGET turbojpeg)

if(NOT TARGET jpeg)
    find_package(Jpeg)
    if(JPEG_FOUND)
        add_library(jpeg SHARED IMPORTED)
        set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${JPEG_LIBRARY})
    else(JPEG_FOUND)
        message(FATAL_ERROR "Cannot find extern library libjpeg")
    endif(JPEG_FOUND)
endif(NOT TARGET jpeg)

if(NOT TARGET proj)
    find_package(Proj)
    if(PROJ_FOUND)
        add_library(proj SHARED IMPORTED)
        set_property(TARGET proj PROPERTY IMPORTED_LOCATION ${PROJ_LIBRARY})
    else(PROJ_FOUND)
        message(FATAL_ERROR "Cannot find extern library proj")
    endif(PROJ_FOUND)
endif(NOT TARGET proj)

IF(CEPH_ENABLED)
    if(NOT TARGET rados)
        find_package(Rados)
        if(RADOS_FOUND)
            add_library(rados SHARED IMPORTED)
            set_property(TARGET rados PROPERTY IMPORTED_LOCATION ${RADOS_LIBRARY})
        else(RADOS_FOUND)
            message(FATAL_ERROR "Cannot find extern library librados")
        endif(RADOS_FOUND)
    endif(NOT TARGET rados)
ENDIF(CEPH_ENABLED)

# Statique

if(UNITTEST_ENABLED)
  
  # Extern libraries, shared

    if(NOT TARGET cppunit)
        find_package(CppUnit)
        if(CPPUNIT_FOUND)
            add_library(cppunit SHARED IMPORTED)
            set_property(TARGET cppunit PROPERTY IMPORTED_LOCATION ${CPPUNIT_LIBRARY})
        else(CPPUNIT_FOUND)
            message(FATAL_ERROR "Cannot find extern library cppunit")
        endif(CPPUNIT_FOUND)
    endif(NOT TARGET cppunit)

endif(UNITTEST_ENABLED)

if(DOC_ENABLED)
  
  # Extern libraries, shared

    if(NOT TARGET Doxygen)
        find_package(Doxygen REQUIRED dot)
        if(DOXYGEN_FOUND)
            message(STATUS "Doxygen ${DOXYGEN_VERSION} found")
        else(DOXYGEN_FOUND)
            message(FATAL_ERROR "Cannot find extern tool doxygen")
        endif(DOXYGEN_FOUND)
    endif(NOT TARGET Doxygen)

endif(DOC_ENABLED)

set(DEPENDENCIES_FOUND TRUE BOOL)
