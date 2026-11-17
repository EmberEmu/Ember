# Copyright (c) 2014 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME rest)
set(LIBRARY_NAME    rest_shared) # Shared library
set(LIBRARY_OBJECTS librest)     # Object library

set(LIBRARY_HDR
    Service.h
)

set(LIBRARY_SRC
    Service.cpp
)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

set(EXECUTABLE_SOURCE
    main.cpp
    ${ICON_RESOURCE}
)

add_library(${LIBRARY_OBJECTS} OBJECT
    ${LIBRARY_SRC}
    ${LIBRARY_HDR}
)

target_include_directories(${LIBRARY_OBJECTS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIBRARY_OBJECTS} PUBLIC Threads::Threads)

add_library(${LIBRARY_NAME} ${SERVICE_LIB_LINKAGE}
    $<TARGET_OBJECTS:${LIBRARY_OBJECTS}>
)

add_dependencies(${LIBRARY_OBJECTS} SPARK_RPC_GENERATE)

if(BUILD_SHARED)
    target_compile_definitions(${LIBRARY_NAME} PUBLIC EXPORT_SERVICE)
	set(LIBRARY_INSTALL_TARGET ${LIBRARY_NAME})
endif()

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_OBJECTS})
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} PUBLIC service PRIVATE ${LIBRARY_OBJECTS} Threads::Threads PUBLIC logger Boost::beast Boost::program_options)

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCE})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared Boost::beast Boost::program_options Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} ${LIBRARY_INSTALL_TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Services/Applications")
set_target_properties(${LIBRARY_OBJECTS} PROPERTIES FOLDER "Services/Libraries")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Shared Service Libraries")