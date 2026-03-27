/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <stdexcept>

namespace ember::protocol::server {

struct Pong final {
	std::uint32_t sequence_id;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> sequence_id;
		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << sequence_id;
		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
