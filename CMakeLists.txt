cmake_minimum_required(VERSION 2.6.0)

# Set the plugin name to build
project(opcua)

# Supported options:
# -DFOGLAMP_INCLUDE
# -DFOGLAMP_LIB
# -DFOGLAMP_SRC
# -DFOGLAMP_INSTALL
#
# If no -D options are given and FOGLAMP_ROOT environment variable is set
# then FogLAMP libraries and header files are pulled from FOGLAMP_ROOT path.

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

# Add here all needed FogLAMP libraries as list
set(NEEDED_FOGLAMP_LIBS common-lib)

# Find source files
file(GLOB SOURCES *.cpp)

# Find FogLAMP includes and libs, by including FindFogLAMP.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(FogLAMP)
# If errors: make clean and remove Makefile
if (NOT FOGLAMP_FOUND)
	if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
		execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
	endif()
	# Stop the build process
	message(FATAL_ERROR "FogLAMP plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FOGLAMP_INCLUDE_DIRS and FOGLAMP_LIB_DIRS variables are set 

# Find the freeopcua files
set(OPCUADIR $ENV{FREEOPCUA})

# Add ./include
include_directories(include)
# Add FogLAMP include dir(s)
include_directories(${FOGLAMP_INCLUDE_DIRS})

# Add other include paths
# We assume the 'freeopcua' header files are available here: 
if (NOT EXISTS "${OPCUADIR}/include")
	message(FATAL_ERROR "OPCUADIR does not apear to be pointing at a valid OPCUA source tree")
	return()
endif()
include_directories(${OPCUADIR}/include)

# Add FogLAMP lib path
link_directories(${FOGLAMP_LIB_DIRS})

# Add additional link directories
# We assume the 'freeopcua' libraries are available here: 
link_directories(~/freeopcua/build/lib)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES} version.h)

# Add FogLAMP library names
target_link_libraries(${PROJECT_NAME} ${NEEDED_FOGLAMP_LIBS})

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

set(FOGLAMP_INSTALL "" CACHE INTERNAL "")
# Install library
if (FOGLAMP_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FOGLAMP_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FOGLAMP_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()
