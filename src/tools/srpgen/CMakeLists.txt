# Copyright (c) 2018 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME srpgen)

set(EXECUTABLE_SRC
    main.cpp
)

if(WIN32)
    set(EXECUTABLE_SRC ${EXECUTABLE_SRC} resources.rc)
endif()

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE srp6 ${Boost_LIBRARIES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(srpgen PROPERTIES FOLDER "Tools")