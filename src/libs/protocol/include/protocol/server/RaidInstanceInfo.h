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
#include <vector>
#include <cstdint>

namespace ember::protocol::server {

struct RaidInstanceInfo final {
	struct RaidInfo {
		std::uint32_t map;
		std::uint32_t reset_time;
		std::uint32_t instance_id;
	};

	std::vector<RaidInfo> info;

	StreamResult read_from_stream(le_stream auto& stream) {
		std::uint32_t count;
		stream >> count;
		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << std::uint32_t(1);
		stream << std::uint32_t(369);
		stream << std::uint32_t(-1);
		stream << std::uint32_t(101920);
		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
