# Copyright (c) 2016 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME protocol)

set(CLIENT_MESSAGES
    include/protocol/client/AuthSession.h
    include/protocol/client/CharacterCreate.h
    include/protocol/client/CharacterDelete.h
    include/protocol/client/CharacterEnum.h
    include/protocol/client/CharacterRename.h
    include/protocol/client/Ping.h
    include/protocol/client/PlayerLogin.h
)

set(SERVER_MESSAGES
    include/protocol/server/AuthChallenge.h
    include/protocol/server/AuthResponse.h
    include/protocol/server/CharacterCreate.h
    include/protocol/server/CharacterDelete.h
    include/protocol/server/CharacterEnum.h
    include/protocol/server/CharacterRename.h
    include/protocol/server/Pong.h
    include/protocol/server/AddonInfo.h
    include/protocol/server/LogoutComplete.h
    include/protocol/server/CharacterLoginFailed.h
)

source_group("Server Messages" FILES ${SERVER_MESSAGES})
source_group("Client Messages" FILES ${CLIENT_MESSAGES})

set(LIBRARY_SRC
    ${SERVER_MESSAGES}
    ${CLIENT_MESSAGES}
    src/Dummy.cpp
    include/protocol/Opcodes.h
    include/protocol/Packet.h
    include/protocol/Packets.h
    include/protocol/ResultCodes.h
    include/protocol/PacketHeaders.h
	include/protocol/SmartMessage.h
)

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(protocol PROPERTIES FOLDER "Libraries")
