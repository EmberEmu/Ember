# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME temp)

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} taskmaster ${Boost_LIBRARIES})

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs DESTINATION ${CMAKE_INSTALL_PREFIX})