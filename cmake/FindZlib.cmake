# Module for locating the zlib library
# Based on Peter Kapec's FindPCRE module as well as Sergiu Dotenco's FindBotan module

INCLUDE (FindPackageHandleStandardArgs)

FIND_PATH (ZLIB_ROOT_DIR
           NAMES include/zlib.h
           PATHS ENV ZLIBROOT
           DOC "zlib root directory")

# Look for the header file.
FIND_PATH (ZLIB_INCLUDE_DIR
           NAMES zlib.h
           HINTS ${ZLIB_ROOT_DIR}
           PATH_SUFFIXES include
           DOC "zlib include directory")

# Look for the libraries.
FIND_LIBRARY (ZLIB_LIBRARY_RELEASE
              NAMES zlib
              HINTS ${ZLIB_ROOT_DIR}
              PATH_SUFFIXES lib
              DOC "zlib release library")

FIND_LIBRARY (ZLIB_LIBRARY_DEBUG
              NAMES zlibd
              HINTS ${ZLIB_ROOT_DIR}
              PATH_SUFFIXES lib
              DOC "zlib debug library")

IF (ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_RELEASE)
  SET (ZLIB_LIBRARY
       optimized ${ZLIB_LIBRARY_RELEASE}
       debug ${ZLIB_LIBRARY_DEBUG} CACHE DOC "zlib library")
ELSEIF (ZLIB_LIBRARY_RELEASE)
  SET (ZLIB_LIBRARY ${ZLIB_LIBRARY_RELEASE} CACHE DOC "zlib library")
ENDIF (ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_RELEASE)

IF (ZLIB_INCLUDE_DIR)
  SET (_ZLIB_VERSION_HEADER ${ZLIB_INCLUDE_DIR}/zlib.h)

  IF (EXISTS ${_ZLIB_VERSION_HEADER})
    FILE (STRINGS ${_ZLIB_VERSION_HEADER} _ZLIB_VERSION_TMP REGEX
      "#define ZLIB_VER_(MAJOR|MINOR|REVISION|SUBREVISION)[ \t]+[0-9]+")

    STRING (REGEX REPLACE
      ".*#define ZLIB_VER_MAJOR[ \t]+([0-9]+).*" "\\1" ZLIB_VER_MAJOR
      ${_ZLIB_VERSION_TMP})
    STRING (REGEX REPLACE
      ".*#define ZLIB_VER_MINOR[ \t]+([0-9]+).*" "\\1" ZLIB_VER_MINOR
      ${_ZLIB_VERSION_TMP})
    STRING (REGEX REPLACE
      ".*#define ZLIB_VER_REVISION[ \t]+([0-9]+).*" "\\1" ZLIB_VER_REVISION
      ${_ZLIB_VERSION_TMP})
    STRING (REGEX REPLACE
      ".*#define ZLIB_VER_SUBREVISION[ \t]+([0-9]+).*" "\\1" ZLIB_VER_SUBREVISION
      ${_ZLIB_VERSION_TMP})

    SET (ZLIB_VERSION_COUNT 4)
    SET (ZLIB_VERSION
      ${ZLIB_VER_MAJOR}.${ZLIB_VER_MINOR}.${ZLIB_VER_REVISION}.${ZLIB_VER_SUBREVISION})
  ENDIF (EXISTS ${_ZLIB_VERSION_HEADER})
ENDIF (ZLIB_INCLUDE_DIR)

# Handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if all listed variables are TRUE.
FIND_PACKAGE_HANDLE_STANDARD_ARGS (ZLIB REQUIRED_VARS ZLIB_ROOT_DIR 
  ZLIB_INCLUDE_DIR ZLIB_LIBRARY VERSION_VAR ZLIB_VERSION)

# Copy the results to the output variables.
IF (ZLIB_FOUND)
	SET (ZLIB_LIBRARIES ${ZLIB_LIBRARY})
	SET (ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
ELSE (ZLIB_FOUND)
	SET (ZLIB_LIBRARIES)
	SET (ZLIB_INCLUDE_DIRS)
ENDIF (ZLIB_FOUND)

MARK_AS_ADVANCED (ZLIB_INCLUDE_DIR ZLIB_LIBRARY
  ZLIB_LIBRARY_DEBUG ZLIB_LIBRARY_RELEASE)
