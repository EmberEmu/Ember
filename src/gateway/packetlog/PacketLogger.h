/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"
#include <game_protocol/Packets.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <spark/buffers/BinaryStream.h>
#include <chrono>
#include <memory>
#include <vector>

namespace ember {

class PacketLogger final {
	std::vector<std::unique_ptr<PacketSink>> sinks_;

public:
	void add_sink(std::unique_ptr<PacketSink> sink);
	void reset();

	void log(const spark::Buffer& buffer, std::size_t length, PacketDirection dir);

	template<typename PacketT>
	void log(const PacketT& packet, PacketDirection dir) {
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::vector<std::uint8_t> buffer(64);
		spark::VectorBufferAdaptor adaptor(buffer);
		spark::BinaryStream stream(adaptor);
		stream << packet;

		for(auto& sink : sinks_) {
			sink->log(buffer, time, dir);
		}
	}
};

} // ember