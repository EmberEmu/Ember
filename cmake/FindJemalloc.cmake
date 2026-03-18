# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Copyright 2023 Google LLC
#
# Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
# you may not use this file except in compliance with the License.  You may
# obtain a copy of the License at
#
# https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# Author: Aleksei Vetrov
#[=======================================================================[.rst:
FindJemalloc
---------
Finds the jemalloc library.
Imported Targets
^^^^^^^^^^^^^^^^
This module provides the following imported targets, if found:
``Jemalloc::Jemalloc``
  The Jemalloc library
Result Variables
^^^^^^^^^^^^^^^^
This will define the following variables:
``Jemalloc_FOUND``
  True if the system has the Jemalloc library.
``Jemalloc_VERSION``
  The version of the Jemalloc library which was found.
``Jemalloc_INCLUDE_DIRS``
  Include directories needed to use Jemalloc.
``Jemalloc_LIBRARIES``
  Libraries needed to link to Jemalloc.
``Jemalloc_DEFINITIONS``
  the compiler switches required for using Jemalloc
Cache Variables
^^^^^^^^^^^^^^^
The following cache variables may also be set:
``Jemalloc_INCLUDE_DIR``
  The directory containing ``jemalloc.h``.
``Jemalloc_LIBRARY``
  The path to the ``libjemalloc.so``.
#]=======================================================================]
find_package(PkgConfig)
pkg_check_modules(PC_Jemalloc QUIET jemalloc)
find_library(
  Jemalloc_LIBRARY
  NAMES jemalloc
  HINTS ${PC_Jemalloc_LIBDIR} ${PC_Jemalloc_LIBRARY_DIRS})
# Try the value from user if the library is not found.
if(DEFINED Jemalloc_LIBRARIES AND NOT DEFINED Jemalloc_LIBRARY)
  set(Jemalloc_LIBRARY ${Jemalloc_LIBRARIES})
endif()
mark_as_advanced(Jemalloc_LIBRARY)
find_path(
  Jemalloc_INCLUDE_DIR
  NAMES jemalloc.h
  PATH_SUFFIXES jemalloc
  HINTS ${PC_Jemalloc_INCLUDEDIR} ${PC_Jemalloc_INCLUDE_DIRS})
# Try the value from user if the library is not found.
if(DEFINED Jemalloc_INCLUDE_DIRS AND NOT DEFINED Jemalloc_INCLUDE_DIR)
  set(Jemalloc_INCLUDE_DIR ${Jemalloc_INCLUDE_DIRS})
endif()
mark_as_advanced(Jemalloc_INCLUDE_DIR)
set(Jemalloc_VERSION ${PC_Jemalloc_VERSION})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Jemalloc
  REQUIRED_VARS Jemalloc_LIBRARY Jemalloc_INCLUDE_DIR
  VERSION_VAR Jemalloc_VERSION)
if(Jemalloc_FOUND)
  set(Jemalloc_LIBRARIES ${Jemalloc_LIBRARY})
  set(Jemalloc_INCLUDE_DIRS ${Jemalloc_INCLUDE_DIR})
  set(Jemalloc_DEFINITIONS ${PC_Jemalloc_CFLAGS_OTHER})
endif()
if(Jemalloc_FOUND AND NOT TARGET Jemalloc::Jemalloc)
  add_library(Jemalloc::Jemalloc UNKNOWN IMPORTED)
  set_target_properties(
    Jemalloc::Jemalloc
    PROPERTIES IMPORTED_LOCATION "${Jemalloc_LIBRARY}"
               INTERFACE_COMPILE_OPTIONS "${PC_Jemalloc_CFLAGS_OTHER}"
               INTERFACE_INCLUDE_DIRECTORIES "${Jemalloc_INCLUDE_DIR}")
endif()