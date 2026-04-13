/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientLogHelper.h"
#include "ClientConnection.h"
#include "ConnectionDefines.h"
#include <protocol/Concepts.h>
#include <logger/Logger.h>

namespace ember::realm {

inline void ClientHandler::send(protocol::is_packet auto& packet) {
	PACKET_TRACE(context_, "<- {}", packet.opcode);
	connection_->send(packet);
}

} // realm, ember