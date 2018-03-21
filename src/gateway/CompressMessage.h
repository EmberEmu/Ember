/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <spark/Buffer.h>

namespace ember {

int compress_message(const protocol::ServerPacket& packet, spark::Buffer& out, int compression_level);
int compress_message(const spark::Buffer& in, spark::Buffer& out, int compression_level);

} // ember