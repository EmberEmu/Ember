# Copyright (c) 2021 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME sparkc)

set(EXECUTABLE_SRC
    main.cpp
    Printer.h
    Printer.cpp
    SchemaParser.h
    SchemaParser.cpp
    )

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} logging shared ${Boost_LIBRARIES} ${flatbuffers} Threads::Threads)
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)