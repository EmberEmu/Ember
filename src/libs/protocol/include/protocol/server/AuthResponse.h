/*
 * Copyright (c) 2015 - 2025 Ember
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

namespace ember::protocol::server {

struct AuthResponse final {
	Result result;
	std::uint32_t queue_position = 0;
	std::uint32_t billing_time = 0;
	std::uint8_t billing_flags = 0;
	std::uint32_t billing_rested = 0;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> result;

		if(result == Result::auth_wait_queue) {
			stream >> queue_position;
		}

		if(result == Result::auth_ok) {
			stream >> billing_time;
			stream >> billing_flags;
			stream >> billing_rested;
		}

		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << result;

		if(result == Result::auth_wait_queue) {
			stream << queue_position;
		}

		if(result == Result::auth_ok) {
			stream << billing_time;
			stream << billing_flags;
			stream << billing_rested;
		}

		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
