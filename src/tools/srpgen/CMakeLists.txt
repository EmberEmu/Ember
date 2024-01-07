# Copyright (c) 2018 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME srpgen)

set(EXECUTABLE_SRC
    main.cpp
    )

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} srp6 ${BOTAN_LIBRARY} ${Boost_LIBRARIES})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(srpgen PROPERTIES FOLDER "Tools")