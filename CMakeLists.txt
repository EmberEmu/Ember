# Copyright (c) 2014 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.30)
project(Ember)

option(BUILD_OPT_TOOLS "Build optional tools" ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin/lib)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(MSVC)
    set(CMAKE_SYSTEM_VERSION 10.0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") # Multiprocessor compilation
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # Multiprocessor compilation
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")  # Enable the conforming preprocessor
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")  # Conformance mode

    # Release build without the optimisations for faster iteration
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;ReleaseUnoptimised" CACHE STRING "Build types" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASEUNOPTIMISED "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)

    option(AVX2 "Enable AVX2 instruction set" ON)

    add_compile_options(
        # Enable intrinsics
        "$<$<CONFIG:Release>:/Oi>"
        "$<$<CONFIG:RelWithDebInfo>:/Oi>"

        # Whole program optimisation
        "$<$<CONFIG:Release>:/GL>"
        "$<$<CONFIG:RelWithDebInfo>:/GL>"

        # Favour speed over size
        "$<$<CONFIG:Release>:/Ot>"
        "$<$<CONFIG:RelWithDebInfo>:/Ot>"

        # Disable optimisations
        "$<$<CONFIG:ReleaseUnoptimised>:/Od>"

        # String pooling
        "$<$<CONFIG:Release>:/GF>"
        "$<$<CONFIG:RelWithDebInfo>:/GF>"

        # Disable buffer security checks
        "$<$<CONFIG:Release>:/GS->"
        "$<$<CONFIG:RelWithDebInfo>:/GS->"
        "$<$<CONFIG:ReleaseUnoptimised>:/GS->"
    )

    # Enable AVX2
    if(AVX2)
        add_compile_options(
            "$<$<CONFIG:Release>:/arch:AVX2>"
            "$<$<CONFIG:RelWithDebInfo>:/arch:AVX2>"
        )
    endif()
    
    add_link_options(
         # Link-time code generation
        "$<$<CONFIG:Release>:/LTCG:INCREMENTAL>"
        "$<$<CONFIG:RelWithDebInfo>:/LTCG:INCREMENTAL>"

        "$<$<CONFIG:Release>:/ignore:4075>"
        "$<$<CONFIG:RelWithDebInfo>:/ignore:4075>"
    )

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.36)
        message(FATAL_ERROR "You need VS2022 17.6.0 or newer to compile Ember on Windows!")
    endif()
endif()

set(BUILD_SHARED_MSG "Builds the services as shared libraries - generated code may be slower but Fusion service reloading is enabled")
set(CMAKE_POSITION_INDEPENDENT_CODE ${BUILD_SHARED})
set(PREALLOC_CLIENTS_THREAD 4) # Preallocate memory for the specified number of clients per thread
set(PREALLOC_NODES_THREAD   4) # Preallocate memory for the specified number of buffer nodes per thread

option(BUILD_SHARED ${BUILD_SHARED_MSG} OFF)
option(ENABLE_FAST_CLIENT_DISPATCH_TABLE "Fast client event dispatch cache at the cost extra per-thread memory consumption" ON)
option(ENABLE_HUGE_PAGES "When running on Linux, enable huge page support (disable if memory constrained)" ON)
option(ENABLE_PAGE_LOCKING "Allow the allocator to lock pages (disable if memory constrained)" ON)
option(ALLOCATOR_TRACKING "Enable viewing custom memory allocator statistics - slow, debugging only!" OFF)

if (ALLOCATOR_TRACKING)
    add_compile_definitions(EMBER_DEBUG_ALLOCATOR_TRACKING)
endif()

if (ENABLE_HUGE_PAGES)
    add_compile_definitions(ENABLE_HUGE_PAGES)
endif()

if (ENABLE_PAGE_LOCKING)
    add_compile_definitions(ENABLE_PAGE_LOCKING)
endif()

if (BUILD_SHARED)
    add_compile_definitions(BUILD_SHARED_SERVICES)
    set(SERVICE_LIB_LINKAGE "SHARED")
else()
    set(SERVICE_LIB_LINKAGE "STATIC")
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
    add_compile_definitions(_WIN32_WINNT=${ver})
endif(WIN32)

option(WITH_JEMALLOC "Compile with Jemalloc, replacing the system allocator" OFF)

if(WITH_JEMALLOC)
    add_compile_definitions(WITH_JEMALLOC)
endif()

##############################
#            Boost           #
##############################
add_compile_definitions(BOOST_ALL_NO_LIB)
add_compile_definitions(BOOST_UUID_FORCE_AUTO_LINK)

if(BUILD_SHARED)
    set(Boost_USE_STATIC_LIBS OFF)
else()
    set(Boost_USE_STATIC_LIBS ON)
endif()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.90.0 REQUIRED COMPONENTS program_options)

##############################
#             Git            #
##############################
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(VERSION --tags --dirty=-d)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/libs/core/banner/Version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" Version.h)
set(version_file "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp")

##############################
#            Botan           #
##############################
find_package(Botan 3.10 REQUIRED)

if(BUILD_SHARED)
    set(BOTAN_TARGET botan::botan)
else()
    set(BOTAN_TARGET botan::botan-static)
endif()

# temporary Botan workarounds
if(TARGET ${BOTAN_TARGET})
    get_target_property(_botan_release_lib ${BOTAN_TARGET} IMPORTED_LOCATION)
    if(_botan_release_lib)
        get_filename_component(_botan_lib_dir "${_botan_release_lib}" DIRECTORY)
        get_filename_component(_botan_lib_name "${_botan_release_lib}" NAME)
        set(_botan_debug_lib "${_botan_lib_dir}/../debug/lib/${_botan_lib_name}")
        if(EXISTS "${_botan_debug_lib}")
            get_filename_component(_botan_debug_lib "${_botan_debug_lib}" ABSOLUTE)
            set_property(TARGET ${BOTAN_TARGET} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS DEBUG RELEASE)
            set_target_properties(${BOTAN_TARGET} PROPERTIES
                IMPORTED_LOCATION_DEBUG   "${_botan_debug_lib}"
                IMPORTED_LOCATION_RELEASE "${_botan_release_lib}")
        endif()
    endif()
endif()

##############################
#     MySQL Connector C++    #
##############################
find_package(MySQLConnectorCPP REQUIRED)
include_directories(${MYSQLCCPP_INCLUDE_DIRS})

##############################
#         FlatBuffers        #
##############################
find_package(FlatBuffers 2.0.8 REQUIRED)
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

if(NOT BUILD_SHARED)
    add_compile_definitions(PCRE_STATIC)
endif()

##############################
#          JEMALLOC          #
##############################
if(WITH_JEMALLOC)
    find_package(Jemalloc REQUIRED)
    link_libraries(Jemalloc::Jemalloc)
endif()

##############################
#          JSONCONS          #
##############################
find_package(jsoncons REQUIRED)

##############################
#         Google Test        #
##############################
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  URL_HASH SHA512=9046841044a2bf7edfd96854ad9c44ffae4fcb9fb59a075b367507c0762a98eb32cb6968d46663228272e26321e96f4dd287c95baa22c6af9bad902b8b6ede4e 
)
set(INSTALL_GTEST OFF)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
set(FETCHCONTENT_FULLY_DISCONNECTED ON)

include(GoogleTest)

add_compile_definitions(DB_MYSQL) # temporary!

include_directories(${CMAKE_SOURCE_DIR}/deps)
include(BuildDBCLoaders)
include(BuildSparkServices)

add_subdirectory(schemas)
set(cmake_ctest_arguments "CTEST_OUTPUT_ON_FAILURE")
enable_testing()
add_subdirectory(tests)
add_subdirectory(src)
add_subdirectory(configs)
add_subdirectory(scripts)
add_subdirectory(deps)

# Enables folder view in Visual Studio solutions
set_property(GLOBAL PROPERTY USE_FOLDERS ON)