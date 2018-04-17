/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packet.h"
#include "Opcodes.h"
#include <game_protocol/server/SMSG_AUTH_CHALLENGE.h>
#include <game_protocol/server/SMSG_AUTH_RESPONSE.h>
#include <game_protocol/server/SMSG_PONG.h>
#include <game_protocol/server/SMSG_CHAR_ENUM.h>
#include <game_protocol/server/SMSG_CHAR_CREATE.h>
#include <game_protocol/server/SMSG_CHAR_DELETE.h>
#include <game_protocol/server/SMSG_CHAR_RENAME.h>
#include <game_protocol/server/SMSG_CHARACTER_LOGIN_FAILED.h>
#include <game_protocol/server/SMSG_LOGOUT_COMPLETE.h>
#include <game_protocol/server/SMSG_ADDON_INFO.h>
#include <game_protocol/client/CMSG_AUTH_SESSION.h>
#include <game_protocol/client/CMSG_PING.h>
#include <game_protocol/client/CMSG_CHAR_CREATE.h>
#include <game_protocol/client/CMSG_CHAR_DELETE.h>
#include <game_protocol/client/CMSG_CHAR_ENUM.h>
#include <game_protocol/client/CMSG_CHAR_RENAME.h>
#include <game_protocol/client/CMSG_PLAYER_LOGIN.h>

namespace ember::protocol {

using SMSG_ADDON_INFO             = ServerPacket<ServerOpcode::SMSG_ADDON_INFO, smsg::SMSG_ADDON_INFO>::Type;
using SMSG_AUTH_CHALLENGE         = ServerPacket<ServerOpcode::SMSG_AUTH_CHALLENGE, smsg::SMSG_AUTH_CHALLENGE>::Type;
using SMSG_AUTH_RESPONSE          = ServerPacket<ServerOpcode::SMSG_AUTH_RESPONSE, smsg::SMSG_AUTH_RESPONSE>::Type;
using SMSG_PONG                   = ServerPacket<ServerOpcode::SMSG_PONG, smsg::SMSG_PONG>::Type;
using SMSG_CHAR_ENUM              = ServerPacket<ServerOpcode::SMSG_CHAR_ENUM, smsg::SMSG_CHAR_ENUM>::Type;
using SMSG_CHAR_CREATE            = ServerPacket<ServerOpcode::SMSG_CHAR_CREATE, smsg::SMSG_CHAR_CREATE>::Type;
using SMSG_CHAR_DELETE            = ServerPacket<ServerOpcode::SMSG_CHAR_DELETE, smsg::SMSG_CHAR_DELETE>::Type;
using SMSG_CHAR_RENAME            = ServerPacket<ServerOpcode::SMSG_CHAR_RENAME, smsg::SMSG_CHAR_RENAME>::Type;
using SMSG_CHARACTER_LOGIN_FAILED = ServerPacket<ServerOpcode::SMSG_CHARACTER_LOGIN_FAILED, smsg::SMSG_CHARACTER_LOGIN_FAILED>::Type;
using SMSG_LOGOUT_COMPLETE        = ServerPacket<ServerOpcode::SMSG_LOGOUT_COMPLETE, smsg::SMSG_LOGOUT_COMPLETE>::Type;

using CMSG_AUTH_SESSION           = ClientPacket<ClientOpcode::CMSG_AUTH_SESSION, cmsg::CMSG_AUTH_SESSION>::Type;
using CMSG_PING                   = ClientPacket<ClientOpcode::CMSG_PING, cmsg::CMSG_PING>::Type;
using CMSG_CHAR_CREATE            = ClientPacket<ClientOpcode::CMSG_CHAR_CREATE, cmsg::CMSG_CHAR_CREATE>::Type;
using CMSG_CHAR_DELETE            = ClientPacket<ClientOpcode::CMSG_CHAR_DELETE, cmsg::CMSG_CHAR_DELETE>::Type;
using CMSG_CHAR_ENUM              = ClientPacket<ClientOpcode::CMSG_CHAR_ENUM, cmsg::CMSG_CHAR_ENUM>::Type;
using CMSG_CHAR_RENAME            = ClientPacket<ClientOpcode::CMSG_CHAR_RENAME, cmsg::CMSG_CHAR_RENAME>::Type;
using CMSG_PLAYER_LOGIN           = ClientPacket<ClientOpcode::CMSG_PLAYER_LOGIN, cmsg::CMSG_PLAYER_LOGIN>::Type;

} // protocol, ember

