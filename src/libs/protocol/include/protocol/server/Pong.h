/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>

namespace ember::protocol::server {

namespace be = boost::endian;

struct Pong final {
	be::little_uint32_t sequence_id;

	StreamResult read_from_stream(auto& stream) try {
		stream >> sequence_id;
		return stream? StreamResult::SUCCESS : StreamResult::FAILED;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << sequence_id;
		return stream? StreamResult::SUCCESS : StreamResult::FAILED;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}
};

} // server, protocol, ember
