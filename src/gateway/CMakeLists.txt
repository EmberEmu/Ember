# Copyright (c) 2016 - 2026 Ember
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
    NetworkListener.h
    ClientConnection.h
    ClientConnection.inl
    AccountClient.h
    RealmQueue.h
    ClientHandler.h
    ClientHandler.inl
    PacketCrypto.h
    ConnectionStats.h
    ServerConfig.h
    QoS.h
    WorldConnection.h
    WorldSessions.h
    WorldClients.h
    CharacterClient.h
    CompressMessage.h
    Routing.h
    ClientLogHelper.h
    ConnectionDefines.h
    SocketType.h
    states/ClientStates.h
    states/Authentication.h
    states/CharacterList.h
    states/WorldForwarder.h
    states/SessionClose.h
    states/WorldTransfer.h
    states/ClientContext.h
    states/StateJumpTables.h
    states/AuthenticationContext.h
    states/WorldEnter.h
    states/WorldEnterContext.h
    states/StateFwdDecl.h
    packet_log/PacketLogger.h
    packet_log/PacketSink.h
    packet_log/FBSink.h
    packet_log/LogSink.h
    Service.h
    LoggingCallbacks.h
)

set(LIBRARY_SRC
    EventDispatcher.cpp
    SessionManager.cpp
    ClientConnection.cpp
    RealmService.cpp
    AccountClient.cpp
    RealmQueue.cpp
    ClientHandler.cpp
    QoS.cpp
    WorldConnection.cpp
    WorldSessions.cpp
    WorldClients.cpp
    CharacterClient.cpp
    CompressMessage.cpp
    NetworkListener.cpp
    states/Authentication.cpp
    states/CharacterList.cpp
    states/WorldForwarder.cpp
    states/SessionClose.cpp
    states/WorldEnter.cpp
    packet_log/PacketLogger.cpp
    packet_log/FBSink.cpp
    packet_log/LogSink.cpp
    Service.cpp
)

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} PRIVATE thread nsd conpool stun ports dbcreader protocol spark ${ZLIB_LIBRARY} Threads::Threads PUBLIC logger commands shared ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(gateway PROPERTIES FOLDER "Services/Applications")
set_target_properties(libgateway PROPERTIES FOLDER "Services/Libraries")