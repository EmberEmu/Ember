# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME gateway)
set(LIBRARY_NAME    libgateway)

set(LIBRARY_HDR
    Config.h
    Event.h
    EventDispatcher.h
    Events.h
    EventTypes.h
    Locator.h
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
    states/SessionClose.h
    states/ClientContext.h
    states/StateLUT.h
    )

set(LIBRARY_SRC
    EventDispatcher.cpp
    Locator.cpp
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
    states/SessionClose.cpp
    states/StateLUT.cpp
    )

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} SPARK_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} dbcreader game_protocol spark logging shared ${BOTAN_LIBRARY} ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} dbcreader game_protocol spark conpool logging shared ${ZLIB_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})