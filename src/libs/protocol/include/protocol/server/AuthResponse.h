/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>

namespace ember::protocol::server {

namespace be = boost::endian;

struct AuthResponse final {
	Result result;
	be::little_uint32_t queue_position = 0;
	be::little_uint32_t billing_time = 0;
	be::little_uint8_t billing_flags = 0;
	be::little_uint32_t billing_rested = 0;

	StreamResult read_from_stream(auto& stream) try {
		stream >> result;

		if(result == Result::AUTH_WAIT_QUEUE) {
			stream >> queue_position;
		}

		if(result == Result::AUTH_OK) {
			stream >> billing_time;
			stream >> billing_flags;
			stream >> billing_rested;
		}

		return StreamResult::SUCCESS;
	} catch(const std::exception&) {
		return StreamResult::FAILED;
	}

	StreamResult write_to_stream(auto& stream) const {
		stream << result;

		if(result == Result::AUTH_WAIT_QUEUE) {
			stream << queue_position;
		}

		if(result == Result::AUTH_OK) {
			stream << billing_time;
			stream << billing_flags;
			stream << billing_rested;
		}

		return StreamResult::SUCCESS;
	}
};

} // server, protocol, ember
