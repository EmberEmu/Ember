# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(SDK_NAME blaze)
set(LIBRARY_NAME blazesdk)

set(LIBRARY_SRC
    src/BlazeSDK.cpp
    src/Client.cpp
)

set(LIBRARY_HDR
	include/ember/blaze/impl/Symbols.h
	include/ember/blaze/impl/Client.h
)

set(LIBRARY_HDR_SHIPPING
    include/ember/blaze/BlazeSDK.h
	include/ember/blaze/Shared.h
)

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SRC} ${LIBRARY_HDR}  ${LIBRARY_HDR_SHIPPING})
target_compile_definitions(${LIBRARY_NAME} PUBLIC EXPORT_SDK)

target_include_directories(${LIBRARY_NAME} PUBLIC  
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "SDKs")

install(FILES ${LIBRARY_HDR_SHIPPING} DESTINATION sdk/${SDK_NAME}/include/ember/blaze)

install(TARGETS ${LIBRARY_NAME}
    EXPORT ${LIBRARY_NAME}Targets 
    LIBRARY DESTINATION sdk/${SDK_NAME}/lib
    ARCHIVE DESTINATION sdk/${SDK_NAME}/lib
    INCLUDES DESTINATION include
)

install(EXPORT ${LIBRARY_NAME}Targets 
    FILE ${LIBRARY_NAME}Targets.cmake
    NAMESPACE ember::
    DESTINATION sdk/${SDK_NAME}/lib/cmake/${LIBRARY_NAME}
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}ConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    ${LIBRARY_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}Config.cmake"
    @ONLY
)

install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}ConfigVersion.cmake"
    DESTINATION sdk/${SDK_NAME}/lib/cmake/${LIBRARY_NAME}
)
