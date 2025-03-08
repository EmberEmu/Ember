/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

namespace be = boost::endian;

struct PlayerLogin final {
	be::little_uint64_t character_id;

	StreamResult read_from_stream(auto& stream) try {
		stream >> character_id;
		return stream? StreamResult::SUCCESS : StreamResult::STREAM_ERROR;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << character_id;
		return stream? StreamResult::SUCCESS : StreamResult::STREAM_ERROR;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}
};

} // client, protocol, ember
