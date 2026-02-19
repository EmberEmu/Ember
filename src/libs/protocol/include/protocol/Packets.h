/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packet.h"
#include "Opcodes.h"
#include <protocol/server/AuthChallenge.h>
#include <protocol/server/AuthResponse.h>
#include <protocol/server/Pong.h>
#include <protocol/server/CharacterEnum.h>
#include <protocol/server/CharacterCreate.h>
#include <protocol/server/CharacterDelete.h>
#include <protocol/server/CharacterRename.h>
#include <protocol/server/CharacterLoginFailed.h>
#include <protocol/server/LogoutComplete.h>
#include <protocol/server/AddonInfo.h>
#include <protocol/client/AuthSession.h>
#include <protocol/client/Ping.h>
#include <protocol/client/CharacterCreate.h>
#include <protocol/client/CharacterDelete.h>
#include <protocol/client/CharacterEnum.h>
#include <protocol/client/CharacterRename.h>
#include <protocol/client/PlayerLogin.h>

namespace ember::protocol {

using smsg_addon_info             = ServerPacket<ServerOpcode::smsg_addon_info,             server::AddonInfo>;
using smsg_auth_challenge         = ServerPacket<ServerOpcode::smsg_auth_challenge,         server::AuthChallenge>;
using smsg_auth_response          = ServerPacket<ServerOpcode::smsg_auth_response,          server::AuthResponse>;
using smsg_pong                   = ServerPacket<ServerOpcode::smsg_pong,                   server::Pong>;
using smsg_char_enum              = ServerPacket<ServerOpcode::smsg_char_enum,              server::CharacterEnum>;
using smsg_char_create            = ServerPacket<ServerOpcode::smsg_char_create,            server::CharacterCreate>;
using smsg_char_delete            = ServerPacket<ServerOpcode::smsg_char_delete,            server::CharacterDelete>;
using smsg_char_rename            = ServerPacket<ServerOpcode::smsg_char_rename,            server::CharacterRename>;
using smsg_character_login_failed = ServerPacket<ServerOpcode::smsg_character_login_failed, server::CharacterLoginFailed>;
using smsg_logout_complete        = ServerPacket<ServerOpcode::smsg_logout_complete,        server::LogoutComplete>;

using cmsg_auth_session           = ClientPacket<ClientOpcode::cmsg_auth_session, client::AuthSession>;
using cmsg_ping                   = ClientPacket<ClientOpcode::cmsg_ping,         client::Ping>;
using cmsg_char_create            = ClientPacket<ClientOpcode::cmsg_char_create,  client::CharacterCreate>;
using cmsg_char_delete            = ClientPacket<ClientOpcode::cmsg_char_delete,  client::CharacterDelete>;
using cmsg_char_enum              = ClientPacket<ClientOpcode::cmsg_char_enum,    client::CharacterEnum>;
using cmsg_char_rename            = ClientPacket<ClientOpcode::cmsg_char_rename,  client::CharacterRename>;
using cmsg_player_login           = ClientPacket<ClientOpcode::cmsg_player_login, client::PlayerLogin>;

} // protocol, ember

