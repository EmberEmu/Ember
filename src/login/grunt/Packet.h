/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "StreamTypes.h"
#include "Opcodes.h"

namespace ember::grunt {

struct Packet {
	enum class State {
		initial,
		call_again,
		done
	};

	Opcode opcode;

	explicit Packet(Opcode opcode) : opcode(opcode) { }

	virtual State read_from_stream(PacketStream& stream) = 0;
	virtual void write_to_stream(PacketStream& stream) const = 0;
	virtual ~Packet() = default;
};

} // client, grunt, ember