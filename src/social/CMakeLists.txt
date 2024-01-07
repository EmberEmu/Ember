# Copyright (c) 2016 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME social)
set(LIBRARY_NAME    libsocial)

set(LIBRARY_HDR
    MonitorCallbacks.h
    FilterTypes.h
    )

set(LIBRARY_SRC
    MonitorCallbacks.cpp
    )

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} logging shared ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} spark logging shared ${Boost_LIBRARIES} Threads::Threads)

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(social libsocial PROPERTIES FOLDER "Services")