# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME gateway)
set(LIBRARY_NAME    libgateway)

set(LIBRARY_HDR
    SessionManager.h
    FilterTypes.h
    RealmService.h
    ServicePool.h
    NetworkListener.h
    ClientConnection.h
    AccountService.h
    RealmQueue.h
    ClientHandler.h
    PacketCrypto.h
    ConnectionStats.h
    ServerConfig.h
    QoS.h
    WorldConnection.h
    WorldSessions.h
    WorldClients.h
    CharacterService.h
    states/ClientStates.h
    states/Authentication.h
    states/CharacterList.h
    states/WorldForwarder.h
    states/InQueue.h
    states/ClientContext.h
    states/StateLUT.cpp
    temp.h
    )

set(LIBRARY_SRC
    SessionManager.cpp
    ClientConnection.cpp
    RealmService.cpp
    ServicePool.cpp
    AccountService.cpp
    RealmQueue.cpp
    ClientHandler.cpp
    QoS.cpp
    WorldConnection.cpp
    WorldSessions.cpp
    WorldClients.cpp
    CharacterService.cpp
    states/Authentication.cpp
    states/CharacterList.cpp
    states/WorldForwarder.cpp
    states/InQueue.cpp
    states/StateLUT.cpp
    temp.cpp
    )

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} game_protocol spark logging shared ${BOTAN_LIBRARY} ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} game_protocol spark conpool logging shared ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs DESTINATION ${CMAKE_INSTALL_PREFIX})