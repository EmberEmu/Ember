# Copyright (c) 2014 - 2022 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.14)
project(Ember)

option(BUILD_OPT_TOOLS "Build optional tools" ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(CMAKE_SYSTEM_VERSION 10.0)
	add_definitions(-D_HAS_AUTO_PTR_ETC=1)               # Until Boost conforms with C++17
	add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING) # Silence erroneous VS2017 warning - check if fixed in VS2019
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")        # Multiprocessor compilation
    
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.13)
		message(FATAL_ERROR "You need VS2017 Update 6 or newer to compile Ember on Windows!")
	endif()
endif()

option(DISABLE_EMBER_THREADS "Disable multithreading for debugging" OFF)

if(DISABLE_EMBER_THREADS)
	add_definitions(-DDEBUG_NO_THREADS)
endif()

find_package(Threads REQUIRED)

# todo, breaks with 64-bit
# http://stackoverflow.com/questions/9742003/platform-detection-in-cmake
if (WIN32)
    macro(get_WIN32_WINNT version)
        if (CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if ("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif ("${verMajor}" MATCHES "10")
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif(CMAKE_SYSTEM_VERSION)
    endmacro(get_WIN32_WINNT)

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif(WIN32)

##############################
#            Boost           #
##############################
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_UUID_FORCE_AUTO_LINK)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.76.0 REQUIRED COMPONENTS program_options system)
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
find_package(Botan 2.14.0 REQUIRED)
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
include(${CMAKE_MODULE_PATH}/BuildFlatBuffers.cmake)

##############################
#            zlib            #
##############################
find_package(ZLIB 1.2.8 REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

##############################
#            PCRE            #
##############################
find_package(PCRE 8.39 REQUIRED)
include_directories(${PCRE_INCLUDE_DIR})
option(PCRE_STATIC_LIB "Statically link to PCRE" ON)

if(PCRE_STATIC_LIB)
  add_definitions(-DPCRE_STATIC)
endif()

##############################
#         Google Test        #
##############################
include(FetchContent)
FetchContent_Declare(
  googletest
  URL ${CMAKE_SOURCE_DIR}/deps/googletest-release-1.11.0.tar.gz
  URL_HASH SHA512=6fcc7827e4c4d95e3ae643dd65e6c4fc0e3d04e1778b84f6e06e390410fe3d18026c131d828d949d2f20dde6327d30ecee24dcd3ef919e21c91e010d149f3a28 
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_definitions(-DDB_MYSQL) #temporary!

include_directories(${CMAKE_SOURCE_DIR}/deps)
include(BuildDBCLoaders) # temp, maybe
include(BuildSparkServices) # temp, maybe

add_subdirectory(schemas)
enable_testing()
add_subdirectory(tests)
add_subdirectory(src)
add_subdirectory(configs)
