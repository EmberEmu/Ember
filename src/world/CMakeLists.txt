# Copyright (c) 2016 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME world)
set(LIBRARY_NAME    libworld)

set(LIBRARY_HDR
    networking/NetworkListener.h
    networking/GatewayClient.h
    networking/SessionManager.h
    )

set(LIBRARY_SRC
    networking/GatewayClient.cpp
    networking/SessionManager.cpp
    )


add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} spark logging shared ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} spark logging shared ${Boost_LIBRARIES})
set_target_properties(world libworld PROPERTIES FOLDER "Services")