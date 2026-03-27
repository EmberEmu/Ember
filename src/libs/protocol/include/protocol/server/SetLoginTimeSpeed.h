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

namespace ember::protocol::server {

struct SetLoginTimeSpeed final {
	std::uint32_t time;
	float speed;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> time;
		stream >> speed;
		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << time;
		stream << speed;
		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
