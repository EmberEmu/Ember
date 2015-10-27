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
#include <boost/optional.hpp>
#include <memory>

namespace ember { namespace grunt {

typedef std::unique_ptr<Packet> PacketHandle;

class Handler {
	enum State {
		NEW_PACKET, INITIAL_READ, CONTINUATION
	};

	PacketHandle curr_packet_;
	State state_ = State::NEW_PACKET;
	std::size_t wire_length = 0;

	void handle_new_packet(spark::Buffer& buffer);
	void handle_continuation(spark::Buffer& buffer);

public:
	boost::optional<PacketHandle> try_deserialise(spark::Buffer& buffer);
};

}} // grunt, ember