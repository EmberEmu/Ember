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

namespace ember::protocol::client {

struct ItemQuerySingle final {
	std::uint32_t item;
	std::uint64_t query;

	StreamResult read_from_stream(le_stream auto& stream) try {
		stream >> item;
		stream >> query;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(le_stream auto& stream) const try {
		stream << item;
		stream << query;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // client, protocol, ember
