# Copyright (c) 2014, 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)

set(GRUNT_SRC
    grunt/Exceptions.h
    grunt/ResultCodes.h
    grunt/Handler.h
    grunt/Handler.cpp
    grunt/Packets.h
    grunt/Packet.h
    grunt/client/LoginChallenge.h
    grunt/client/LoginProof.h
    grunt/client/Opcodes.h
    grunt/client/ReconnectProof.h
    grunt/client/RequestRealmList.h
    grunt/server/LoginChallenge.h
    grunt/server/LoginProof.h
    grunt/server/RealmList.h
    grunt/server/Opcodes.h
)

set(EXECUTABLE_HDR
    ../shared/Banner.h
    ../shared/memory/ASIOAllocator.h
    ../shared/database/daos/UserDAO.h
    ../shared/database/daos/IPBanDAO.h
    ../shared/IPBanCache.h
    ../shared/misc/PacketStream.h
    ../shared/threading/ThreadPool.h
    ../shared/util/LogConfig.h
    PacketHandler.h
    SessionBuilders.h
    LoginSession.h
    LoginPacketCheck.h
    NetworkListener.h
    NetworkSession.h
    LoginHandler.h
    LoginHandlerBuilder.h
    PacketBuffer.h
    Protocol.h
    GameVersion.h
    Authenticator.h
    RealmList.h
    Actions.h
    Patcher.h
    )

set(EXECUTABLE_SRC
    ../shared/threading/ThreadPool.cpp
    ../shared/Banner.cpp
    ../shared/util/LogConfig.cpp
    LoginHandler.cpp
    LoginPacketCheck.cpp
    SessionManager.cpp
    LoginSession.cpp
    RealmList.cpp
    Authenticator.cpp
    Patcher.cpp
    main.cpp
    )

source_group("Grunt Protocol" FILES ${GRUNT_SRC})

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_HDR} ${EXECUTABLE_SRC} ${GRUNT_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} spark conpool srp6 logging ${BOTAN_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/login/ DESTINATION ${CMAKE_INSTALL_PREFIX})