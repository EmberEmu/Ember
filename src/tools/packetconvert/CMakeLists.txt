# Copyright (c) 2018 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME packetconvert)

set(EXECUTABLE_SRC
    main.cpp
    StreamReader.h
    StreamReader.cpp
    Sink.h
    ConsoleSink.h
    ConsoleSink.cpp
    )

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC} ${version_file})
add_dependencies(${EXECUTABLE_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${EXECUTABLE_NAME} shared game_protocol ${Boost_LIBRARIES})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)