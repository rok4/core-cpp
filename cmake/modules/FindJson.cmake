
# CMake module to search for Jsoncpp library
#
# If it's found it sets JSON_FOUND to TRUE
# and following variables are set:
#    JSON_INCLUDE_DIR
#    JSON_LIBRARY

FIND_PATH(JSON_INCLUDE_DIR json11.hpp
    /usr/local/include 
    /usr/include/x86_64-linux-gnu/
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/src
    )

FIND_LIBRARY(JSON_LIBRARY NAMES libjson11.a PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64 
    /usr/local/lib/jsoncpp/json 
    /usr/lib/jsoncpp/json 
    c:/msys/local/lib
    C:/dev/cpp/src
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Json" DEFAULT_MSG JSON_INCLUDE_DIR JSON_LIBRARY )
