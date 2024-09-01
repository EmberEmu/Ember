# Module for locating the MySQL Connector/C++ library
# Based on Sergiu Dotenco's FindBotan module

include(FindPackageHandleStandardArgs)

set(_MYSQLCCPP_POSSIBLE_LIB_SUFFIXES lib)

find_path(MYSQLCCPP_ROOT_DIR
          NAMES include/mysql_connection.h
          PATHS ENV MYSQLCCPPROOT
          DOC "MySQL Connector/C++ root directory")

find_path(MYSQLCCPP_INCLUDE_DIR
          NAMES mysql_connection.h
          HINTS ${MYSQLCCPP_ROOT_DIR}
          PATH_SUFFIXES include
          DOC "MySQL Connector/C++ include directory")

find_library(MYSQLCCPP_LIBRARY_RELEASE
             NAMES mysqlcppconn
             HINTS ${MYSQLCCPP_ROOT_DIR}
             PATH_SUFFIXES ${_MYSQLCCPP_POSSIBLE_LIB_SUFFIXES}
             DOC "MySQL Connector/C++ release library")

find_library(MYSQLCCPP_LIBRARY_DEBUG
             NAMES mysqlcppconnd
             HINTS ${MYSQLCCPP_ROOT_DIR}
             PATH_SUFFIXES ${_MYSQLCCPP_POSSIBLE_LIB_SUFFIXES}
             DOC "MySQL Connector/C++ debug library")

if(MYSQLCCPP_LIBRARY_DEBUG AND MYSQLCCPP_LIBRARY_RELEASE)
  set(MYSQLCCPP_LIBRARY
      optimized ${MYSQLCCPP_LIBRARY_RELEASE}
      debug ${MYSQLCCPP_LIBRARY_DEBUG} CACHE DOC "MySQL Connector/C++ library")
elseif(MYSQLCCPP_LIBRARY_RELEASE)
  set(MYSQLCCPP_LIBRARY ${MYSQLCCPP_LIBRARY_RELEASE} CACHE STRING "MySQL Connector/C++ library")
endif(MYSQLCCPP_LIBRARY_DEBUG AND MYSQLCCPP_LIBRARY_RELEASE)

set(MYSQLCCPP_INCLUDE_DIRS ${MYSQLCCPP_INCLUDE_DIR})
set(MYSQLCCPP_LIBRARIES ${MYSQLCCPP_LIBRARY})

mark_as_advanced(MYSQLCCPP_ROOT_DIR MYSQLCCPP_INCLUDE_DIR MYSQLCCPP_LIBRARY
  MYSQLCCPP_LIBRARY_DEBUG MYSQLCCPP_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(MySQLConnectorCPP REQUIRED_VARS MYSQLCCPP_ROOT_DIR
  MYSQLCCPP_INCLUDE_DIR MYSQLCCPP_LIBRARY)