/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"
#include <game_protocol/Packet.h>
#include <spark/buffers/Buffer.h>
#include <memory>
#include <vector>

namespace ember {

class PacketLogger final {
	std::vector<std::unique_ptr<PacketSink>> sinks_;

public:
	void add_sink(std::unique_ptr<PacketSink> sink);
	void reset();

	void log(const spark::Buffer& buffer, std::size_t length, PacketDirection dir);
	void log(const protocol::ServerPacket& packet, PacketDirection dir);
};

} // ember