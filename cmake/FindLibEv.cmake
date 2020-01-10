# Find LibEv
#
# Once done, this will define:
#
#  LIBEV_FOUND - system has Event
#  LIBEV_INCLUDE_DIRS - the Event include directories
#  LIBEV_LIBRARIES - link these to use Event
#

if (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARY)
	# Already in cache, be silent
	set(LIBEV_FIND_QUIETLY TRUE)
endif (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARY)

find_path(LIBEV_INCLUDE_DIR ev.h
	PATHS /usr/include
	PATH_SUFFIXES ev
)
find_library(LIBEV_LIBRARY
	NAMES ev
	PATHS /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEv
	REQUIRED_VARS
		LIBEV_INCLUDE_DIR
		LIBEV_LIBRARY
)

if(NOT LIBEV_INCLUDE_DIR OR NOT LIBEV_LIBRARY)
	message(STATUS "libev not found")
else(NOT LIBEV_INCLUDE_DIR OR NOT LIBEV_LIBRARY)
	set(LIBEV_FOUND TRUE)
endif(NOT LIBEV_INCLUDE_DIR OR NOT LIBEV_LIBRARY)

if(LIBEV_FOUND)
	set(LIBEV_LIBRARIES ${LIBEV_LIBRARY})
endif(LIBEV_FOUND)

mark_as_advanced(LIBEV_INCLUDE_DIR LIBEV_LIBRARY)
