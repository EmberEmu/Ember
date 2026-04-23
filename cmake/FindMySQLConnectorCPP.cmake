# Module for locating the MySQL Connector/C++ library
# Based on Sergiu Dotenco's FindBotan module

include(FindPackageHandleStandardArgs)

set(MYSQLCCPP_ROOT_HINTS
    include/mysql_connection.h            # Docker
    include/jdbc/mysql_connection.h)      # vcpkg
set(MYSQLCCPP_INCLUDE_PATH
    include                               # Docker
    include/jdbc)                         # vcpkg

set(MYSQLCCPP_LIBRARY_NAMES
    mysqlcppconn                          # Docker
    mysqlcppconn-static                   # vcpkg (Windows)
    libmysqlcppconn-static.a)             # vcpkg (Linux)
set(_MYSQLCCPP_POSSIBLE_LIB_SUFFIXES lib)

set(MYSQLCCPP_LIBRARYD_NAMES
    mysqlcppconnd                         # Docker
    mysqlcppconn-static                   # vcpkg (Windows)
    libmysqlcppconn-static.a)             # vcpkg (Linux)
set(_MYSQLCCPP_POSSIBLE_LIBD_SUFFIXES
    debug/lib                             # vcpkg
    lib)                                  # Docker

find_path(MYSQLCCPP_ROOT_DIR
          NAMES ${MYSQLCCPP_ROOT_HINTS}
          PATHS ENV MYSQLCCPPROOT
          DOC "MySQL Connector/C++ root directory")

find_path(MYSQLCCPP_INCLUDE_DIR
          NAMES mysql_connection.h
          HINTS ${MYSQLCCPP_ROOT_DIR}
          PATH_SUFFIXES ${MYSQLCCPP_INCLUDE_PATH}
          DOC "MySQL Connector/C++ include directory")

find_library(MYSQLCCPP_LIBRARY_RELEASE
             NAMES ${MYSQLCCPP_LIBRARY_NAMES}
             HINTS ${MYSQLCCPP_ROOT_DIR}
             PATH_SUFFIXES ${_MYSQLCCPP_POSSIBLE_LIB_SUFFIXES}
             DOC "MySQL Connector/C++ release library")

find_library(MYSQLCCPP_LIBRARY_DEBUG
             NAMES ${MYSQLCCPP_LIBRARYD_NAMES}
             HINTS ${MYSQLCCPP_ROOT_DIR}
             PATH_SUFFIXES ${_MYSQLCCPP_POSSIBLE_LIBD_SUFFIXES}
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

# When using vcpkg, pull in the connector-jdbc imported target so its
# transitive deps (libmysql, openssl, zlib, lz4, zstd, protobuf, dnsapi)
# get linked alongside mysqlcppconn-static.lib.
unset(UNOFFICIAL_MYSQL_CONNECTOR_CPP_INITIALIZED CACHE)
find_package(unofficial-mysql-connector-cpp CONFIG QUIET)
if(TARGET unofficial::mysql-connector-cpp::connector-jdbc)
    list(APPEND MYSQLCCPP_LIBRARY unofficial::mysql-connector-cpp::connector-jdbc)
    list(APPEND MYSQLCCPP_LIBRARIES unofficial::mysql-connector-cpp::connector-jdbc)
    list(REMOVE_DUPLICATES MYSQLCCPP_LIBRARY)
    list(REMOVE_DUPLICATES MYSQLCCPP_LIBRARIES)
    set(MYSQLCCPP_LIBRARY "${MYSQLCCPP_LIBRARY}" CACHE STRING "MySQL Connector/C++ library" FORCE)
endif()