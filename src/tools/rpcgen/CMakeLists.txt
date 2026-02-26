# Copyright (c) 2021 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME rpcgen)

set(EXECUTABLE_SRC
    main.cpp
    SchemaParser.h
    SchemaParser.cpp
    Utility.h
    Utility.cpp
)

if(WIN32)
    set(EXECUTABLE_SRC ${EXECUTABLE_SRC} resources.rc)
endif()

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
add_dependencies(${EXECUTABLE_NAME} FB_SCHEMA_COMPILE)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/deps/nlohmann)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE logger shared ${Boost_LIBRARIES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Tools")