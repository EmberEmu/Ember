# Copyright (c) 2021 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME mdns)
set(LIBRARY_NAME    libmdns)

set(LIBRARY_HDR
    ServiceRegistry.h
    MulticastSocket.h
    Parser.h
    Server.h
    Socket.h
    DNSDefines.h
    Handler.h
    )

set(LIBRARY_SRC
    ServiceRegistry.cpp
    MulticastSocket.cpp
    Parser.cpp
    Server.cpp
    )


add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} logging shared ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} logging shared ${Boost_LIBRARIES})