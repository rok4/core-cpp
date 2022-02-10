################### RECHERCHE DES DÉPENDANCES

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

include(cmake/modules/dependencies.cmake)

################### DÉFINITION DES FICHIERS SOURCES

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

if(FILEIMAGE_ENABLED)
    
    file(GLOB TMP_SRCS "src/image/file/*.cpp")
    set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} ${TMP_SRCS})
    
    if(KDU_ENABLED)
        set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} "src/image/file/kakadu/LibkakaduImage.cpp")
    endif(KDU_ENABLED)

endif(FILEIMAGE_ENABLED)

if(OBJECT_ENABLED)
    file(GLOB TMP_SRCS "src/storage/object/*.cpp")
    set(LIBROK4CORE_SRCS ${LIBROK4CORE_SRCS} ${TMP_SRCS})
endif(OBJECT_ENABLED)

add_library(rok4core ${LIBROK4CORE_SRCS})

################### LIEN DES DÉPENDANCES

target_link_libraries(rok4core PUBLIC proj thread turbojpeg jpeg json zlib curl openssl crypto boostlog)
if(FILEIMAGE_ENABLED)
    target_link_libraries(rok4core PUBLIC jpeg2000 tiff png)
endif(FILEIMAGE_ENABLED)

if(OBJECT_ENABLED)
    target_link_libraries(rok4core PUBLIC rados)
endif(OBJECT_ENABLED)


# Permet d'avoir la configuration du projet (version, librairie JPEG2000, mode objet..)
target_include_directories(rok4core PUBLIC "${PROJECT_BINARY_DIR}")


