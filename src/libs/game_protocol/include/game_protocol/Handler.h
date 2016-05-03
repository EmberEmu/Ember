/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <spark/Buffer.h>
#include <boost/optional.hpp>
#include <memory>

namespace ember { namespace protocol {

typedef std::unique_ptr<Packet> PacketHandle;

class Handler {
	enum State {
		NEW_PACKET, READ
	};

	PacketHandle curr_packet_;
	State state_ = State::NEW_PACKET;

	bool handle_new_packet(spark::Buffer& buffer);
	void handle_read(spark::Buffer& buffer);

public:
	boost::optional<PacketHandle> try_deserialise(spark::Buffer& buffer, bool* error);
};

}} // grunt, ember