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
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

namespace be = boost::endian;

struct Ping final {
	be::little_uint32_t sequence_id;
	be::little_uint32_t latency;

	StreamResult read_from_stream(auto& stream) try {
		stream >> sequence_id;
		stream >> latency;
		return StreamResult::SUCCESS;
	} catch(const std::exception&) {
		return StreamResult::FAILED;
	}

	StreamResult write_to_stream(auto& stream) const {
		stream << sequence_id;
		stream << latency;
		return StreamResult::SUCCESS;
	}
};

} // client, protocol, ember
