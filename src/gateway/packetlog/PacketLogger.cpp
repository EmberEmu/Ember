/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PacketLogger.h"
#include <spark/buffers/ChainedBuffer.h>
#include <spark/buffers/BinaryStream.h>
#include <chrono>

namespace sc = std::chrono;

namespace ember {

void PacketLogger::add_sink(std::unique_ptr<PacketSink> sink) {
	sinks_.emplace_back(std::move(sink));
}

void PacketLogger::reset() {
	sinks_.clear();
}

void PacketLogger::log(const protocol::ServerPacket& packet, PacketDirection dir) {
	spark::ChainedBuffer<128> buffer;
	spark::BinaryStream stream(buffer);
	stream << packet.opcode << packet;
	log(buffer, buffer.size(), dir);
}

void PacketLogger::log(const spark::Buffer& buffer, std::size_t length, PacketDirection dir) {
	const auto time = sc::system_clock::to_time_t(sc::system_clock::now());

	for(auto& sink : sinks_) {
		sink->log(buffer, length, time, dir);
	}
}

} // ember