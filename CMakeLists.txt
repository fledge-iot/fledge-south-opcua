cmake_minimum_required(VERSION 2.6.0)

# Set the plugin name to build
project(opcua)

# Supported options:
# -DFLEDGE_INCLUDE
# -DFLEDGE_LIB
# -DFLEDGE_SRC
# -DFLEDGE_INSTALL
#
# If no -D options are given and FLEDGE_ROOT environment variable is set
# then Fledge libraries and header files are pulled from FLEDGE_ROOT path.

set(CMAKE_CXX_FLAGS "-std=c++11 -O3")

# Generation version header file
set_source_files_properties(version.h PROPERTIES GENERATED TRUE)
add_custom_command(
  OUTPUT version.h
  DEPENDS ${CMAKE_SOURCE_DIR}/VERSION
  COMMAND ${CMAKE_SOURCE_DIR}/mkversion ${CMAKE_SOURCE_DIR}
  COMMENT "Generating version header"
  VERBATIM
)
include_directories(${CMAKE_BINARY_DIR})

# Set plugin type (south, north, filter)
set(PLUGIN_TYPE "south")

# Add here all needed Fledge libraries as list
set(NEEDED_FLEDGE_LIBS common-lib)

# Find source files
file(GLOB SOURCES *.cpp)

# Find Fledge includes and libs, by including FindFledge.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Fledge)
# If errors: make clean and remove Makefile
if (NOT FLEDGE_FOUND)
	if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
		execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
	endif()
	# Stop the build process
	message(FATAL_ERROR "Fledge plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FLEDGE_INCLUDE_DIRS and FLEDGE_LIB_DIRS variables are set 

# Find the freeopcua files
if (NOT "$ENV{FREEOPCUA}" STREQUAL "")
	set(OPCUADIR $ENV{FREEOPCUA})
else()
	set(OPCUADIR "$ENV{HOME}/freeopcua")
endif()

# Add ./include
include_directories(include)
# Add Fledge include dir(s)
include_directories(${FLEDGE_INCLUDE_DIRS})

# Add other include paths
# We assume the 'freeopcua' header files are available here: 
if (NOT EXISTS "${OPCUADIR}/include")
	message(FATAL_ERROR "OPCUADIR does notppear to be pointing at a valid OPCUA source tree")
	return()
endif()
include_directories(${OPCUADIR}/include)

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})

# Add additional link directories
# We assume the 'freeopcua' libraries are available here: 
link_directories(~/freeopcua/build/lib)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES} version.h)

# Add Fledge library names
target_link_libraries(${PROJECT_NAME} ${NEEDED_FLEDGE_LIBS})

# Add additional libraries
target_link_libraries(${PROJECT_NAME} -lpthread -ldl)
# Add freeopcua libraries
find_library(OPCUAPROTOCOL opcuaprotocol ${OPCUADIR}/build/lib)
if (NOT OPCUAPROTOCOL)
	message(FATAL_ERROR "Free OPCUA library opcuaprotocol not found.\n"
			"Please build freeopcua and set the environment variable FREEOPCUA to root of OPCUA")
	return()
endif()
find_library(OPCUACORE opcuacore ${OPCUADIR}/build/lib)
if (NOT OPCUACORE)
	message(FATAL_ERROR "Free OPCUA library opcuacore not found.\n"
			"Please build freeopcua and set the environment variable FREEOPCUA to root of OPCUA")
	return()
endif()
find_library(OPCUACLIENT opcuaclient ${OPCUADIR}/build/lib)
if (NOT OPCUACLIENT)
	message(FATAL_ERROR "Free OPCUA library opcuaclient not found.\n"
			"Please build freeopcua and set the environment variable FREEOPCUA to root of OPCUA")
	return()
endif()
target_link_libraries(${PROJECT_NAME} ${OPCUACLIENT} ${OPCUACORE} ${OPCUAPROTOCOL})

# Set the build version 
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)

set(FLEDGE_INSTALL "" CACHE INTERNAL "")
# Install library
if (FLEDGE_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()
