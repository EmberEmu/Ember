# Copyright (c) 2016 - 2018 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME character)
set(LIBRARY_NAME    libcharacter)

set(LIBRARY_HDR
    CharacterHandler.h
    InventoryTypes.h
    Service.h
    FilterTypes.h
    )

set(LIBRARY_SRC
    CharacterHandler.cpp
    Service.cpp
    )

include_directories(${CMAKE_SOURCE_DIR}/deps/utf8cpp)

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} dbcreader spark logging protocol shared ${BOTAN_LIBRARY} ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} protocol dbcreader spark conpool logging shared  ${PCRE_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})