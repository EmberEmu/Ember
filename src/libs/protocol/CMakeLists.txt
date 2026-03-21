# Copyright (c) 2016 - 2026 Ember
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
    include/protocol/client/NameQuery.h
    include/protocol/client/SetActiveMover.h
    include/protocol/client/QueryTime.h
    include/protocol/client/ItemQuerySingle.h
    include/protocol/client/QueryNextMailTime.h
    include/protocol/client/BattlefieldStatus.h
    include/protocol/client/MeetingStoneInfo.h
    include/protocol/client/MoveTimeSkipped.h
    include/protocol/client/MoveFallLand.h
    include/protocol/client/ZoneUpdate.h
    include/protocol/client/UpdateAccountData.h
    include/protocol/client/JoinChannel.h
    include/protocol/client/GenericMove.h
    include/protocol/client/MessageChat.h
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
    include/protocol/server/LoginVerifyWorld.h
    include/protocol/server/TutorialFlags.h
    include/protocol/server/UpdateObject.h
    include/protocol/server/NameQueryResponse.h
    include/protocol/server/SplineMoveSetWalkMode.h
    include/protocol/server/QueryTimeResponse.h
    include/protocol/server/SetLoginTimeSpeed.h
    include/protocol/server/RaidInstanceInfo.h
    include/protocol/server/ItemQuerySingleResponse.h
    include/protocol/server/QueryNextMailTime.h
    include/protocol/server/BattlefieldStatus.h
    include/protocol/server/MeetingStoneSetQueue.h
    include/protocol/server/Motd.h
    include/protocol/server/MoveTimeSkipped.h
    include/protocol/server/MoveFallLand.h
    include/protocol/server/Weather.h
    include/protocol/server/AccountDataTimes.h
    include/protocol/server/GenericMove.h
    include/protocol/server/MessageChat.h
    include/protocol/server/TriggerCinematic.h
)

set(CORE_HEADERS
    include/protocol/Opcodes.h
    include/protocol/Packet.h
    include/protocol/Packets.h
    include/protocol/ResultCodes.h
    include/protocol/PacketHeaders.h
    include/protocol/MessageView.h
    include/protocol/Concepts.h
    include/protocol/StreamResult.h
    include/protocol/MovementInfo.h
)

set(HEADERS
    ${CLIENT_MESSAGES}
    ${SERVER_MESSAGES}
    ${CORE_HEADERS}
)

source_group("Client Messages" FILES ${CLIENT_MESSAGES})
source_group("Server Messages" FILES ${SERVER_MESSAGES})
source_group("Core Headers" FILES ${CORE_HEADERS})

add_library(${LIBRARY_NAME} INTERFACE)
target_sources(${LIBRARY_NAME} INTERFACE ${HEADERS})
target_include_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} INTERFACE shared spark ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")