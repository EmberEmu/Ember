/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

struct CharacterEnum final {
	StreamResult read_from_stream(auto& stream) try {
		return stream? StreamResult::success : StreamResult::stream_error;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(auto& stream) const try {
		return stream? StreamResult::success : StreamResult::stream_error;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // client, protocol, ember
