# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME gateway)

set(EXECUTABLE_HDR
    ../shared/Banner.h
    ../shared/util/LogConfig.h
    )

set(EXECUTABLE_SRC
    ../shared/Banner.cpp
    ../shared/util/LogConfig.cpp
    main.cpp
    )

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_HDR} ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} ${MYSQLCCPP_LIBRARY})
target_link_libraries(${EXECUTABLE_NAME} logging)
target_link_libraries(${EXECUTABLE_NAME} conpool)
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/login/ DESTINATION ${CMAKE_INSTALL_PREFIX})