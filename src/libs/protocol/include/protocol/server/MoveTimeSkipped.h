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
#include <protocol/types/PackedGuid.h>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct MoveTimeSkipped final {
	PackedGuid guid;
	std::uint32_t lag;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> guid;
		stream >> lag;
		return StreamResult::success;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << guid;
		stream << lag;
		return StreamResult::success;
	}
};

} // server, protocol, ember
