
if (NOT DEFINED CMAKE_PROJECT_NAME)

    ################### INITIALISATION

    message(STATUS "ROK4 Core built alone")
    # check the cmake version
    cmake_minimum_required(VERSION 3.10)

    # specify the C++ standard
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(BUILD_SHARED_LIBS OFF)

    # Définit le nom du projet 
    project(rok4-core)

    message(STATUS ${CMAKE_PROJECT_NAME})
    message(STATUS ${PROJECT_NAME})

    message(STATUS "Compilation autonome de la lib core ROK4")
    configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

    ################### PARAMÈTRES

    option(OBJECT_ENABLED "Build with object storage" 1)
    option(KDU_ENABLED "Build with kakadu" 0)
    option(KDU_THREADING "Threading level for kakadu" 0)
    option(UNITTEST_ENABLED "Unit tests compilation" 1)
    option(FILEIMAGE_ENABLED "Build FileImage part" 1)

    set(BUILT_ALONE 1)

else (NOT DEFINED CMAKE_PROJECT_NAME)

    message(STATUS "ROK4 Core built from other project")

endif (NOT DEFINED CMAKE_PROJECT_NAME)

################### RECHERCHE DES DÉPENDANCES

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

include(cmake/modules/dependencies.cmake)

################### DÉFINITION DES FICHIERS SOURCES ET DES HEADERS

include_directories(src)

file(GLOB LIBROK4CORE_SRCS
    "src/compressors/*.cpp"
    "src/datastream/*.cpp"
    "src/datasource/*.cpp"
    "src/enums/*.cpp"
    "src/style/*.cpp"
    "src/processors/*.cpp"
    "src/storage/*.cpp"
    "src/utils/*.cpp"
    "src/image/*.cpp"
)

include_directories(${PROJECT_BINARY_DIR} ${PROJ_INCLUDE_DIR} ${TURBOJPEG_INCLUDE_DIR} ${JSON_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${BOOSTLOG_INCLUDE_DIR})

if(FILEIMAGE_ENABLED)
    
    file(GLOB TMP_SRCS "src/image/file/*.cpp")
    set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} ${TMP_SRCS})
    include_directories(${PNG_INCLUDE_DIR} ${TIFF_INCLUDE_DIR})
    
    if(KDU_ENABLED)
        set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} "src/image/file/kakadu/LibkakaduImage.cpp")
        include_directories(${KAKADU_INCLUDE_DIR})
    else(KDU_ENABLED)
        set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} "src/image/file/openjpeg/LibopenjpegImage.cpp")
        include_directories(${OPENJPEG_INCLUDE_DIR})
    endif(KDU_ENABLED)

endif(FILEIMAGE_ENABLED)

if(OBJECT_ENABLED)
    file(GLOB TMP_SRCS "src/storage/object/*.cpp")
    set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} ${TMP_SRCS})
    include_directories(${RADOS_INCLUDE_DIR})
endif(OBJECT_ENABLED)

add_library(rok4core ${LIBROK4CORE_SRCS})

################### LIEN DES DÉPENDANCES

target_link_libraries(rok4core PUBLIC proj turbojpeg jpeg json zlib curl openssl crypto boostlog)
if(FILEIMAGE_ENABLED)
    target_link_libraries(rok4core PUBLIC jpeg2000 tiff png)
endif(FILEIMAGE_ENABLED)

if(OBJECT_ENABLED)
    target_link_libraries(rok4core PUBLIC rados)
endif(OBJECT_ENABLED)

########################################

if(BUILT_ALONE)
    include_directories(${CPPUNIT_INCLUDE_DIR})
    enable_testing()
    add_definitions(-DUNITTEST)
    file(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/CppUnit*.cpp" )
    add_executable(UnitTester-rok4core tests/main.cpp ${UnitTests_SRCS} tests/TimedTestListener.cpp tests/XmlTimedTestOutputterHook.cpp )
    target_link_libraries(UnitTester-rok4core cppunit rok4core)
    foreach(test ${UnitTests_SRCS})
          message("  - adding test ${test}")
          get_filename_component(TestName ${test} NAME_WE)
          add_test(${TestName} UnitTester-rok4core ${TestName})
    endforeach(test)
endif(BUILT_ALONE)
