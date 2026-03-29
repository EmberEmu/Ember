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
#include <array>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct TutorialFlags final {
	std::array<std::uint32_t, 8> flags {
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
	};

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> flags;
		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << flags;
		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
