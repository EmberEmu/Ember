# Copyright (c) 2014, 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)

set(EXECUTABLE_HDR
    LoginHandlerBuilder.h
    LoginPacketCheck.h
    NetworkHandler.h
    LoginHandler.h
    PacketBuffer.h
    Session.h
    Protocol.h
    GameVersion.h
    Authenticator.h
    RealmList.h
    Actions.h
    Patcher.h
    )

set(EXECUTABLE_SRC
    LoginHandler.cpp
    LoginPacketCheck.cpp
    RealmList.cpp
    Authenticator.cpp
    Patcher.cpp
    main.cpp
    )

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_HDR} ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} conpool srp6 logging shared ${BOTAN_LIBRARY} ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/login/ DESTINATION ${CMAKE_INSTALL_PREFIX})