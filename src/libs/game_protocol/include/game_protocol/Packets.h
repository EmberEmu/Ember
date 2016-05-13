/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/server/SMSG_AUTH_CHALLENGE.h>
#include <game_protocol/server/SMSG_AUTH_RESPONSE.h>
#include <game_protocol/server/SMSG_PONG.h>
#include <game_protocol/server/SMSG_CHAR_ENUM.h>
#include <game_protocol/server/SMSG_CHAR_CREATE.h>
#include <game_protocol/server/SMSG_CHARACTER_LOGIN_FAILED.h>
#include <game_protocol/server/SMSG_LOGOUT_COMPLETE.h>

#include <game_protocol/client/CMSG_AUTH_SESSION.h>
#include <game_protocol/client/CMSG_PING.h>
#include <game_protocol/client/CMSG_CHAR_CREATE.h>
#include <game_protocol/client/CMSG_CHAR_DELETE.h>
#include <game_protocol/client/CMSG_CHAR_ENUM.h>
#include <game_protocol/client/CMSG_PLAYER_LOGIN.h>