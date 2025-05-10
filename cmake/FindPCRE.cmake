# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET(PCRE_POSSIBLE_STATICLIB_NAMES
  pcre.lib                                  #         Win
  pcre.a                                    # Linux/macOS
  libpcre.lib                               #         Win (alternate)
  libpcre.a)                                # Linux/macOS (alternate)
SET(PCRE_POSSIBLE_LIBRARY_NAMES
  pcre.dll                                  #         Win
  pcre.so                                   #       Linux
  pcre.dylib                                #       macOS
  libpcre.dll                               #         Win (alternate)
  libpcre.so                                #       Linux (alternate)
  libpcre.dylib)                            #       macOS (alternate)
SET(PCRE_POSSIBLE_LIB_SUFFIX lib)
SET(PCRE_POSSIBLE_LIBD_SUFFIX debug/lib)

FIND_PATH(PCRE_INCLUDE_DIR                  # Look for the header file.
  NAMES pcre.h
  PATH_SUFFIXES include
  DOC "pcre include directory")

FIND_LIBRARY(PCRE_STATICLIB_RELEASE         # Look for the static release library.
  NAMES ${PCRE_POSSIBLE_STATICLIB_NAMES}
  PATH_SUFFIXES ${PCRE_POSSIBLE_LIB_SUFFIX}
  DOC "pcre static release library")

FIND_LIBRARY(PCRE_STATICLIB_DEBUG           # Look for the static debug library.
  NAMES ${PCRE_POSSIBLE_STATICLIB_NAMES}
  PATH_SUFFIXES ${PCRE_POSSIBLE_LIBD_SUFFIX}
  DOC "pcre static debug library")

# Set the library with a preference for a full static set
IF(PCRE_STATICLIB_RELEASE)                  # Found static release
  IF(PCRE_STATICLIB_DEBUG)                  # Found static   debug
    SET(PCRE_LIBRARY                        # Default to static libraries if available.
      optimized ${PCRE_STATICLIB_RELEASE}
      debug ${PCRE_STATICLIB_DEBUG}
      CACHE INTERNAL "PCRE static release and debug libraries")
  ELSE()
    SET(PCRE_LIBRARY ${PCRE_STATICLIB_RELEASE} CACHE INTERNAL "PCRE static release library")
  ENDIF()
ELSE()                                      # Static libraries unavailable - fallback on shared
  FIND_LIBRARY(PCRE_LIBRARY_RELEASE         # Look for the shared release library.
    NAMES ${PCRE_POSSIBLE_LIBRARY_NAMES}
    PATH_SUFFIXES ${PCRE_POSSIBLE_LIB_SUFFIX}
    DOC "pcre release library")
  
  FIND_LIBRARY(PCRE_LIBRARY_DEBUG           # Look for the shared debug library.
    NAMES ${PCRE_POSSIBLE_LIBRARY_NAMES}
    PATH_SUFFIXES ${PCRE_POSSIBLE_LIBD_SUFFIX}
    DOC "pcre debug library")

  IF(PCRE_LIBRARY_RELEASE)                  # Found shared release
    IF(PCRE_LIBRARY_DEBUG)                  # Found shared   debug
      SET(PCRE_LIBRARY                      # Fallback to shared library if available.
        optimized ${PCRE_LIBRARY_RELEASE}
        debug ${PCRE_LIBRARY_DEBUG}
        CACHE INTERNAL "PCRE shared release and debug libraries")
    ELSE()
      SET(PCRE_LIBRARY ${PCRE_LIBRARY_RELEASE} CACHE INTERNAL "PCRE shared release library")
    ENDIF()
  ELSE()                                    # No library available in any expected path.
    MESSAGE(FATAL_ERROR "pcre.h found but the library is unavailable in any of the expected paths.")
  ENDIF()
ENDIF()

# Extract the version from the header and compare it to the required version.
IF(PCRE_INCLUDE_DIR)
  SET(_PCRE_VERSION_HEADER "${PCRE_INCLUDE_DIR}/pcre.h")
  IF(EXISTS "${_PCRE_VERSION_HEADER}")
    FILE(STRINGS "${_PCRE_VERSION_HEADER}" _PCRE_VERSION_TMP REGEX "#define PCRE_(MAJOR|MINOR)[ \t]+[0-9]+")
    STRING(REGEX REPLACE ".*#define PCRE_MAJOR[ \t]+([0-9]+).*" "\\1" _PCRE_FOUND_MAJOR "${_PCRE_VERSION_TMP}")
    STRING(REGEX REPLACE ".*#define PCRE_MINOR[ \t]+([0-9]+).*" "\\1" _PCRE_FOUND_MINOR "${_PCRE_VERSION_TMP}")
    SET(PCRE_FOUND_VERSION "${_PCRE_FOUND_MAJOR}.${_PCRE_FOUND_MINOR}")
    IF(PCRE_FOUND_VERSION VERSION_LESS "${PCRE_VERSION}")
      MESSAGE(FATAL_ERROR "Your version of PCRE is ${PCRE_FOUND_VERSION} - This project requires at least ${PCRE_VERSION}")
    ELSE()
      SET(PCRE_VERSION "${PCRE_FOUND_VERSION}")
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR "Unknown pcre version or pcre.h unavailable in any of the expected paths.")
  ENDIF()
ENDIF()

# Handle the QUIETLY and REQUIRED arguments and mark PCRE as found if all listed variables are TRUE.
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE REQUIRED_VARS PCRE_INCLUDE_DIR PCRE_LIBRARY VERSION_VAR PCRE_VERSION)

# Report which library (set) is being utilized
IF(PCRE_STATICLIB_DEBUG)
  MESSAGE(STATUS "Using PCRE static release and debug libraries.")
ELSEIF(PCRE_STATICLIB_RELEASE)
  MESSAGE(STATUS "Using PCRE static release library.")
ELSEIF(PCRE_LIBRARY_DEBUG)
  MESSAGE(STATUS "Using PCRE shared release and debug libraries.")
ELSEIF(PCRE_LIBRARY_RELEASE)
  MESSAGE(STATUS "Using PCRE shared release library.")
ENDIF()

# Hide internally used variables except for PCRE_LIBRARY and PCRE_INCLUDE_DIR
MARK_AS_ADVANCED(PCRE_POSSIBLE_STATICLIB_NAMES PCRE_POSSIBLE_LIBRARY_NAMES PCRE_POSSIBLE_LIB_SUFFIX
                 PCRE_POSSIBLE_LIBD_SUFFIX PCRE_STATICLIB_RELEASE PCRE_STATICLIB_DEBUG PCRE_LIBRARY_RELEASE 
                 PCRE_LIBRARY_DEBUG _PCRE_VERSION_HEADER _PCRE_VERSION_TMP _PCRE_FOUND_MAJOR _PCRE_FOUND_MINOR 
                 PCRE_FOUND_VERSION PCRE_VERSION)
