# Copyright (c) 2014, 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)
set(LIBRARY_NAME    liblogin)

set(GRUNT_SRC
    grunt/Exceptions.h
    grunt/ResultCodes.h
    grunt/Handler.h
    grunt/Handler.cpp
    grunt/Packets.h
    grunt/Packet.h
    grunt/Opcodes.h
    grunt/client/LoginChallenge.h
    grunt/client/LoginProof.h
    grunt/client/ReconnectProof.h
    grunt/client/RequestRealmList.h
    grunt/server/LoginChallenge.h
    grunt/server/ReconnectChallenge.h
    grunt/server/LoginProof.h
    grunt/server/ReconnectProof.h
    grunt/server/RealmList.h
)

set(LIBRARY_HDR
    Actions.h
    SessionBuilders.h
    LoginSession.h
    NetworkListener.h
    NetworkSession.h
    LoginHandler.h
    LoginHandlerBuilder.h
    SessionManager.h
    GameVersion.h
    Authenticator.h
    RealmList.h
    Actions.h
    Patcher.h
    FilterTypes.h
    MonitorCallbacks.h
    AccountService.h
    )

set(LIBRARY_SRC
    LoginHandler.cpp
    SessionManager.cpp
    LoginSession.cpp
    RealmList.cpp
    Authenticator.cpp
    Patcher.cpp
    MonitorCallbacks.cpp
    AccountService.cpp
    )

source_group("Grunt Protocol" FILES ${GRUNT_SRC})

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC} ${GRUNT_SRC})
target_link_libraries(${LIBRARY_NAME} spark srp6 logging shared ${BOTAN_LIBRARY} ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} spark conpool srp6 logging shared ${BOTAN_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/login/ DESTINATION ${CMAKE_INSTALL_PREFIX})