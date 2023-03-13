################### INITIALISATION

message(STATUS "ROK4 C++ library built")
# check the cmake version
cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

################### PARAMÈTRES

set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation location")
set(BUILD_VERSION "0.0.0" CACHE STRING "Build version")
set(CEPH_ENABLED 0 CACHE BOOL "Build with ceph storage")
set(KDU_ENABLED 0 CACHE BOOL "Build with kakadu")
set(KDU_THREADING 0 CACHE STRING "Threading level for kakadu")
set(UNITTEST_ENABLED 1 CACHE BOOL "Unit tests compilation")
set(DOC_ENABLED 1 CACHE BOOL "Documentation compilation")
set(CPACK_SYSTEM_NAME "ubuntu20.04" CACHE STRING "Building OS, to deduce package format to generate")
set(CPACK_PACKAGE_NAME "librok4-dev" CACHE STRING "Package's name")

set(VERSION ${BUILD_VERSION})
message("Version ${VERSION}")

# Définit le nom du projet 
project(rok4
    VERSION ${VERSION}
    DESCRIPTION "ROK4 library"
    LANGUAGES CXX)

configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

################### RECHERCHE DES DÉPENDANCES

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

include(cmake/modules/dependencies.cmake)

################### DÉFINITION DES FICHIERS SOURCES ET DES HEADERS

file(GLOB LIBROK4_SRCS
    "src/compressors/*.cpp"
    "src/datastream/*.cpp"
    "src/datasource/*.cpp"
    "src/enums/*.cpp"
    "src/style/*.cpp"
    "src/processors/*.cpp"
    "src/storage/*.cpp"
    "src/utils/*.cpp"
    "src/image/*.cpp"
    "src/image/file/*.cpp"
    "src/storage/*.cpp"
)

include_directories(${PROJECT_BINARY_DIR} ${PROJ_INCLUDE_DIR} ${TURBOJPEG_INCLUDE_DIR} ${JSON_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${BOOSTLOG_INCLUDE_DIR})

if(KDU_ENABLED)
    set(LIBROK4_SRCS ${LIBROK4_SRCS} "src/image/file/kakadu/LibkakaduImage.cpp")
    include_directories(${KAKADU_INCLUDE_DIR})
else(KDU_ENABLED)
    set(LIBROK4_SRCS ${LIBROK4_SRCS} "src/image/file/openjpeg/LibopenjpegImage.cpp")
    include_directories(${OPENJPEG_INCLUDE_DIR})
endif(KDU_ENABLED)

if(CEPH_ENABLED)
    set(LIBROK4_SRCS ${LIBROK4_SRCS} "src/storage/ceph/CephPoolContext.cpp")
    include_directories(${RADOS_INCLUDE_DIR})
endif(CEPH_ENABLED)

add_library(${PROJECT_NAME} SHARED ${LIBROK4_SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})

################### LIEN DES DÉPENDANCES

target_link_libraries(${PROJECT_NAME} PUBLIC proj turbojpeg jpeg json zlib curl openssl crypto boostlog jpeg2000 tiff png)

if(CEPH_ENABLED)
    target_link_libraries(${PROJECT_NAME} PUBLIC rados)
endif(CEPH_ENABLED)

################### TESTS UNITAIRES

if(UNITTEST_ENABLED)
    message("${CPPUNIT_INCLUDE_DIR}")
    include_directories(${CPPUNIT_INCLUDE_DIR})
    enable_testing()
    add_definitions(-DUNITTEST)
    file(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/CppUnit*.cpp" )
    add_executable(UnitTester-${PROJECT_NAME} tests/main.cpp ${UnitTests_SRCS} tests/TimedTestListener.cpp tests/XmlTimedTestOutputterHook.cpp )
    target_link_libraries(UnitTester-${PROJECT_NAME} cppunit ${PROJECT_NAME})
    foreach(test ${UnitTests_SRCS})
          message("  - adding test ${test}")
          get_filename_component(TestName ${test} NAME_WE)
          add_test(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    endforeach(test)
endif(UNITTEST_ENABLED)

################### DOCUMENTATION

if(DOC_ENABLED)

    set(DOXYGEN_DOXYFILE_ENCODING UTF-8)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    set(DOXYGEN_OUTPUT_LANGUAGE french)
    set(DOXYGEN_INPUT_ENCODING UTF-8)
    set(DOXYGEN_IMAGE_PATH docs/images)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE docs/main.doxygen.md)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_HEADER docs/doxygen/header.html)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/doxygen-awesome.css)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
        
    doxygen_add_docs(doc ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/)

endif(DOC_ENABLED)

################### INSTALLATION

# For access to standard installation directory variables (CMAKE_INSTALL_xDIR).
include(GNUInstallDirs)

# Set library shared object and API header file to install.
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/rok4
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

################### PACKAGE CONFIGURATION
message(STATUS "Configuring packages")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUILD_ARCHITECTURE "amd64")
else()
    set(BUILD_ARCHITECTURE "i386")
endif()

set(CPACK_GENERATOR "TGZ")
if(CEPH_ENABLED)
    set(CPACK_PACKAGE_FILE_NAME "librok4-ceph-${VERSION}-${CPACK_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
else(CEPH_ENABLED)
    set(CPACK_PACKAGE_FILE_NAME "librok4-base-${VERSION}-${CPACK_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
endif(CEPH_ENABLED)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_VENDOR "IGN")
set(CPACK_PACKAGE_CONTACT "IGN - Géoportail (<tout_rdev@ign.fr>)")

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "ubuntu-20.04" OR CPACK_SYSTEM_NAME STREQUAL "ubuntu-22.04")
    set(CPACK_GENERATOR "DEB;TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g-dev, libcurl4-openssl-dev, libproj-dev, libssl-dev, libturbojpeg0-dev, libjpeg-dev, libc6-dev, libjson11-1-dev, libboost-log-dev, libboost-filesystem-dev, libboost-system-dev, libsqlite3-dev, libpng-dev, libtiff5-dev")
    if(NOT KDU_ENABLED)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libopenjp2-7-dev")
    endif(NOT KDU_ENABLED)
    if(CEPH_ENABLED)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, librados-dev")
    endif(CEPH_ENABLED)
    set(CPACK_BINARY_DEB "ON")
endif()

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "alpine3")
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)


