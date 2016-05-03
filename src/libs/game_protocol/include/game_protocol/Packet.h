/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/BinaryStream.h>
#include <cstdint>

namespace ember { namespace protocol {

struct Packet {
	enum class State {
		INITIAL, CALL_AGAIN, DONE
	};

	virtual State read_from_stream(spark::BinaryStream& stream) = 0;
	virtual void write_to_stream(spark::BinaryStream& stream) const = 0;
	virtual std::uint16_t size() const = 0;
	virtual ~Packet() = default;
};

}} // client, grunt, ember