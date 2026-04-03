/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::client {

struct MoveSetRawPosition final {
	float x;
	float y;
	float z;
	float o;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> x >> y >> z >> o;
		return stream? StreamResult::success : StreamResult::stream_error;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << x << y << z << o;
		return stream? StreamResult::success : StreamResult::stream_error;
	}
};

} // client, protocol, ember
