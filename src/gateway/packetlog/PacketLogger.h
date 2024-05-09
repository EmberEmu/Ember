/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"
#include <protocol/Packets.h>
#include <spark/buffers/BufferAdaptor.h>
#include <spark/buffers/BinaryStream.h>
#include <boost/container/small_vector.hpp>
#include <chrono>
#include <memory>
#include <span>

namespace ember {

class PacketLogger final {
	constexpr static auto RESERVE_LEN = 128u;

	std::vector<std::unique_ptr<PacketSink>> sinks_;

public:
	void add_sink(std::unique_ptr<PacketSink> sink);
	void reset();

	void log(std::span<const std::uint8_t> buffer, PacketDirection dir);
	void log(const spark::Buffer& buffer, std::size_t length, PacketDirection dir);

	template<protocol::is_packet PacketType>
	void log(const PacketType& packet, PacketDirection dir) {
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		boost::container::small_vector<std::uint8_t, RESERVE_LEN> buffer(64);
		spark::BufferAdaptor adaptor(buffer);
		spark::BinaryStream stream(adaptor);
		stream << packet;

		for(auto& sink : sinks_) {
			sink->log(buffer, time, dir);
		}
	}
};

} // ember