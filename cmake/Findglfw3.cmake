# Locate the glfw3 library
#
# This module defines the following variables:
#
# GLFW3_LIBRARY the name of the library;
# GLFW3_INCLUDE_DIR where to find glfw include files.
# GLFW3_FOUND true if both the GLFW3_LIBRARY and GLFW3_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you can define a
# variable called GLFW3_ROOT which points to the root of the glfw library
# installation.
#
# default search dirs
# 
# Cmake file from: https://github.com/daw42/glslcookbook

set( _glfw3_HEADER_SEARCH_DIRS
"C:/GLFW/glfw-3.4/include" )
set( _glfw3_LIB_SEARCH_DIRS
"C:/GLFW/glfw-3.4/lib-mingw-w64" )

# Check environment for root search directory
set( _glfw3_ENV_ROOT $ENV{GLFW3_ROOT} )
if( NOT GLFW3_ROOT AND _glfw3_ENV_ROOT )
	set(GLFW3_ROOT ${_glfw3_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( GLFW3_ROOT )
	list( INSERT _glfw3_HEADER_SEARCH_DIRS 0 "${GLFW3_ROOT}/include" )
	list( INSERT _glfw3_LIB_SEARCH_DIRS 0 "${GLFW3_ROOT}/lib" )
endif()

# Search for the header
find_path(GLFW3_INCLUDE_DIR
    NAMES GLFW/glfw3.h
    PATHS ${_glfw3_HEADER_SEARCH_DIRS}
    NO_DEFAULT_PATH
)

# Search for the library
FIND_LIBRARY(GLFW3_LIBRARY NAMES glfw3 glfw
PATHS ${_glfw3_LIB_SEARCH_DIRS} )
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLFW3 DEFAULT_MSG
GLFW3_LIBRARY GLFW3_INCLUDE_DIR)