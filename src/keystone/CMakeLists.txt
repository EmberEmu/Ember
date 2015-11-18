# Copyright (c) 2014, 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME keystone)
set(LIBRARY_NAME    libkeystone)


set(LIBRARY_HDR
    FilterTypes.h
    )

set(LIBRARY_SRC
    Dummy.cpp
    )


add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
set_target_properties(${LIBRARY_NAME} PROPERTIES PREFIX "")
target_link_libraries(${LIBRARY_NAME} spark logging shared ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp TestService.h)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} spark conpool logging shared ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})