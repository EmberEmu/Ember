# Copyright (c) 2016 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME character)
set(LIBRARY_NAME    libcharacter)

set(LIBRARY_HDR
    CharacterHandler.h
    InventoryTypes.h
    FilterTypes.h
    CharacterService.h
    Service.h
    Config.h
    )

set(LIBRARY_SRC
    CharacterHandler.cpp
    CharacterService.cpp
    Service.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/deps/utf8cpp)

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} dbcreader spark logger protocol shared botan::botan-static ${Boost_LIBRARIES} Threads::Threads)

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} protocol dbcreader spark conpool logger shared ${PCRE_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES} Threads::Threads)

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(character PROPERTIES FOLDER "Services/Applications")
set_target_properties(libcharacter PROPERTIES FOLDER "Services/Libraries")
