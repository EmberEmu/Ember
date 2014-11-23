# Copyright (c) 2014 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0.1)
project(Ember)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18)
		message(FATAL_ERROR "You need VS2013 or newer to compile Ember on Windows!")
	endif()
endif()


##############################
#            Boost           #
##############################
add_definitions(-DBOOST_ALL_NO_LIB)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.56.0 REQUIRED COMPONENTS program_options log system)
include_directories(${Boost_INCLUDE_DIRS})

##############################
#             Git            #
##############################
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(VERSION --tags --dirty=-d)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/Version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" @ONLY)
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

##############################
#         Google Test        #
##############################
ADD_SUBDIRECTORY(deps/gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

##############################
#         Unit Tests         #
##############################
add_executable(unit_tests tests/testgtest.cpp)
target_link_libraries(unit_tests gtest gtest_main)
add_test(unit_tests unit_tests)

add_subdirectory(src)
