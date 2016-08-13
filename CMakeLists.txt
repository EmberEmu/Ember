# Copyright (c) 2014, 2015, 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.1.3)
project(Ember)

option(BUILD_TOOLS "Build tools" ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_CXX_STANDARD 14)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19)
		message(FATAL_ERROR "You need VS2015 or newer to compile Ember on Windows!")
	endif()
endif()

option(DISABLE_EMBER_THREADS "Disable multithreading for debugging" OFF)

if(DISABLE_EMBER_THREADS)
	add_definitions(-DDEBUG_NO_THREADS)
endif()

find_package(Threads REQUIRED)

##############################
#            Boost           #
##############################
add_definitions(-DBOOST_ALL_NO_LIB)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.61.0 REQUIRED COMPONENTS program_options filesystem locale system)
include_directories(${Boost_INCLUDE_DIRS})

##############################
#             Git            #
##############################
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(VERSION --tags --dirty=-d)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/libs/shared/shared/Version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" Version.h)
set(version_file "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp")

##############################
#            Botan           #
##############################
find_package(Botan REQUIRED)
include_directories(${BOTAN_INCLUDE_DIRS})

##############################
#     MySQL Connector C++    #
##############################
find_package(MySQLConnectorCPP REQUIRED)
include_directories(${MYSQLCCPP_INCLUDE_DIRS})

##############################
#         FlatBuffers        #
##############################
find_package(FlatBuffers REQUIRED)
include_directories(${FLATBUFFERS_INCLUDE_DIR})

##############################
#            zlib            #
##############################
find_package(zlib 1.2.8 REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

##############################
#            PCRE            #
##############################
find_package(PCRE 8.39 REQUIRED)
include_directories(${PCRE_INCLUDE_DIR})

##############################
#         Google Test        #
##############################
ADD_SUBDIRECTORY(deps/gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	option(gtest_force_shared_crt ON)
endif()

add_definitions(-DDB_MYSQL) #temporary!

add_subdirectory(tests)
add_subdirectory(src)
