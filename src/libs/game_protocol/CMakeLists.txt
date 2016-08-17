# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME game_protocol)

set(CLIENT_MESSAGES
    include/game_protocol/client/CMSG_AUTH_sESSION.h
    include/game_protocol/client/CMSG_CHAR_CREATE.h
    include/game_protocol/client/CMSG_CHAR_DELETE.h
    include/game_protocol/client/CMSG_CHAR_ENUM.h
    include/game_protocol/client/CMSG_CHAR_RENAME.h
    include/game_protocol/client/CMSG_PING.h
    include/game_protocol/client/CMSG_PLAYER_LOGIN.h
)

set(SERVER_MESSAGES
    include/game_protocol/server/SMSG_AUTH_CHALLENGE.h
    include/game_protocol/server/SMSG_AUTH_RESPONSE.h
    include/game_protocol/server/SMSG_CHAR_CREATE.h
    include/game_protocol/server/SMSG_CHAR_DELETE.h
    include/game_protocol/server/SMSG_CHAR_ENUM.h
    include/game_protocol/server/SMSG_CHAR_RENAME.h
    include/game_protocol/server/SMSG_PONG.h
    include/game_protocol/server/SMSG_ADDON_INFO.h
    include/game_protocol/server/SMSG_LOGOUT_COMPLETE.h
    include/game_protocol/server/SMSG_CHARACTER_LOGIN_FAILED.h
)

source_group("Server Messages" FILES ${SERVER_MESSAGES})
source_group("Client Messages" FILES ${CLIENT_MESSAGES})


set(LIBRARY_SRC
    ${SERVER_MESSAGES}
    ${CLIENT_MESSAGES}
    src/Dummy.cpp
    include/game_protocol/Opcodes.h
    include/game_protocol/Packet.h
    include/game_protocol/Packets.h
    include/game_protocol/ResultCodes.h
    include/game_protocol/PacketHeaders.h
)

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
