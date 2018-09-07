# This CMake file locates the FogLAMP header files and libraries
#
# The following variables are set:
# FOGLAMP_INCLUDE_DIRS - Path(s) to FogLAMP headers files found
# FOGLAMP_LIB_DIRS - Path to FogLAMP shared libraries
# FOGLAMP_SUCCESS - Set on succes
#
# In case of error use SEND_ERROR and return()
#

# Set defaults paths of installed FogLAMP SDK package
set(FOGLAMP_DEFAULT_INCLUDE_DIR "/usr/include/foglamp" CACHE INTERNAL "")
set(FOGLAMP_DEFAULT_LIB_DIR "/usr/lib/foglamp" CACHE INTERNAL "")

# CMakeLists.txt options
set(FOGLAMP_SRC "" CACHE INTERNAL "")
set(FOGLAMP_INCLUDE "" CACHE INTERNAL "")
set(FOGLAMP_LIB "" CACHE INTERNAL "")

# Return variables
set(FOGLAMP_INCLUDE_DIRS "" CACHE INTERNAL "")
set(FOGLAMP_LIB_DIRS "" CACHE INTERNAL "")
set(FOGLAMP_FOUND "" CACHE INTERNAL "")

# No options set
# If FOGLAMP_ROOT env var is set, use it
if (NOT FOGLAMP_SRC AND NOT FOGLAMP_INCLUDE AND NOT FOGLAMP_LIB)
	if (DEFINED ENV{FOGLAMP_ROOT})
		message(STATUS "No options set.\n" 
			"   +Using found FOGLAMP_ROOT $ENV{FOGLAMP_ROOT}")
		set(FOGLAMP_SRC $ENV{FOGLAMP_ROOT})
	endif()
endif()

# -DFOGLAMP_SRC=/some_path or FOGLAMP_ROOT path
# Set return variable FOGLAMP_INCLUDE_DIRS
if (FOGLAMP_SRC)
	unset(_INCLUDE_LIST CACHE)
	file(GLOB_RECURSE _INCLUDE_COMMON "${FOGLAMP_SRC}/C/common/*.h")
	file(GLOB_RECURSE _INCLUDE_SERVICES "${FOGLAMP_SRC}/C/services/common/*.h")
	file(GLOB_RECURSE _INCLUDE_PLUGINS_FILTER_COMMON "${FOGLAMP_SRC}/C/plugins/filter/common/*.h")
	list(APPEND _INCLUDE_LIST ${_INCLUDE_COMMON} ${_INCLUDE_SERVICES})
	foreach(_ITEM ${_INCLUDE_LIST})
		get_filename_component(_ITEM_PATH ${_ITEM} DIRECTORY)
		list(APPEND FOGLAMP_INCLUDE_DIRS ${_ITEM_PATH})
	endforeach()
	list(APPEND FOGLAMP_INCLUDE_DIRS "${FOGLAMP_SRC}/C/thirdparty/rapidjson/include")
	unset(INCLUDE_LIST CACHE)

	list(REMOVE_DUPLICATES FOGLAMP_INCLUDE_DIRS)

	string (REPLACE ";" "\n   +" DISPLAY_PATHS "${FOGLAMP_INCLUDE_DIRS}")
	if (NOT DEFINED ENV{FOGLAMP_ROOT})
		message(STATUS "Using -DFOGLAMP_SRC option for includes\n   +" "${DISPLAY_PATHS}")
	else()
		message(STATUS "Using FOGLAMP_ROOT for includes\n   +" "${DISPLAY_PATHS}")
	endif()

	if (NOT FOGLAMP_INCLUDE_DIRS)
		message(SEND_ERROR "Needed FogLAMP header files not found in path ${FOGLAMP_SRC}/C")
		return()
	endif()
else()
	# -DFOGLAMP_INCLUDE=/some_path
	if (NOT FOGLAMP_INCLUDE)
		set(FOGLAMP_INCLUDE ${FOGLAMP_DEFAULT_INCLUDE_DIR})
		message(STATUS "Using FogLAMP dev package includes " ${FOGLAMP_INCLUDE})
	else()
		message(STATUS "Using -DFOGLAMP_INCLUDE option " ${FOGLAMP_INCLUDE})
	endif()
	# Remove current value from cache
	unset(_FIND_INCLUDES CACHE)
	# Get up to date var from find_path
	find_path(_FIND_INCLUDES NAMES plugin_api.h PATHS ${FOGLAMP_INCLUDE})
	if (_FIND_INCLUDES)
		list(APPEND FOGLAMP_INCLUDE_DIRS ${_FIND_INCLUDES})
	endif()
	# Remove current value from cache
	unset(_FIND_INCLUDES CACHE)

	if (NOT FOGLAMP_INCLUDE_DIRS)
		message(SEND_ERROR "Needed FogLAMP header files not found in path ${FOGLAMP_INCLUDE}")
		return()
	endif()
endif()

#
# FogLAMP Libraries
#
# Check -DFOGLAMP_LIB=/some path is valid
# or use FOGLAMP_SRC/cmake_build/C/lib
# FOGLAMP_SRC might have been set to FOGLAMP_ROOT above
#
if (FOGLAMP_SRC)
	# Set return variable FOGLAMP_LIB_DIRS
        set(FOGLAMP_LIB "${FOGLAMP_SRC}/cmake_build/C/lib")

	if (NOT DEFINED ENV{FOGLAMP_ROOT})
		message(STATUS "Using -DFOGLAMP_SRC option for libs \n   +" "${FOGLAMP_SRC}/cmake_build/C/lib")
	else()
		message(STATUS "Using FOGLAMP_ROOT for libs \n   +" "${FOGLAMP_SRC}/cmake_build/C/lib")
	endif()

	if (NOT EXISTS "${FOGLAMP_SRC}/cmake_build")
		message(SEND_ERROR "FogLAMP has not been built yet in ${FOGLAMP_SRC}  Compile it first.")
		return()
	endif()

	# Set return variable FOGLAMP_LIB_DIRS
	set(FOGLAMP_LIB_DIRS "${FOGLAMP_SRC}/cmake_build/C/lib")
else()
	if (NOT FOGLAMP_LIB)
		set(FOGLAMP_LIB ${FOGLAMP_DEFAULT_LIB_DIR})
		message(STATUS "Using FogLAMP dev package libs " ${FOGLAMP_LIB})
	else()
		message(STATUS "Using -DFOGLAMP_LIB option " ${FOGLAMP_LIB})
	endif()
	# Set return variable FOGLAMP_LIB_DIRS
	set(FOGLAMP_LIB_DIRS ${FOGLAMP_LIB})
endif()

# Check NEEDED_FOGLAMP_LIBS in libraries in FOGLAMP_LIB_DIRS
# NEEDED_FOGLAMP_LIBS variables comes from CMakeLists.txt
foreach(_LIB ${NEEDED_FOGLAMP_LIBS})
	# Remove current value from cache
	unset(_FOUND_LIB CACHE)
	# Get up to date var from find_library
	find_library(_FOUND_LIB NAME ${_LIB} PATHS ${FOGLAMP_LIB_DIRS})
	if (_FOUND_LIB)
		# Extract path form founf library file
		get_filename_component(_DIR_LIB ${_FOUND_LIB} DIRECTORY)
	else()
		message(SEND_ERROR "Needed FogLAMP library ${_LIB} not found in ${FOGLAMP_LIB_DIRS}")
		return()
	endif()
	# Remove current value from cache
	unset(_FOUND_LIB CACHE)
endforeach()

# Set return variable FOGLAMP_FOUND
set(FOGLAMP_FOUND "true")
