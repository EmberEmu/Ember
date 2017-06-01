/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packets.h"
#include "Exceptions.h"
#include <spark/Buffer.h>
#include <logger/Logging.h>
#include <memory>
#include <optional>
#include <cstddef>

namespace ember { namespace grunt {

typedef std::unique_ptr<Packet> PacketHandle;

class Handler {
	enum State {
		NEW_PACKET, READ
	};

	PacketHandle curr_packet_;
	State state_ = State::NEW_PACKET;

	log::Logger* logger_;

	void handle_new_packet(spark::Buffer& buffer);
	void handle_read(spark::Buffer& buffer, std::size_t offset);
	void dump_bad_packet(const spark::buffer_underrun& e, spark::Buffer& buffer, std::size_t offset);

public:
	explicit Handler(log::Logger* logger) : logger_(logger) { }

	std::optional<PacketHandle> try_deserialise(spark::Buffer& buffer);
};

}} // grunt, ember