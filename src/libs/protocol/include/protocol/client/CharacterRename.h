/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <shared/utility/UTF8String.h>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

struct CharacterRename final {
	std::uint64_t id;
	utf8_string name;

	StreamResult read_from_stream(le_stream auto& stream) try {
		stream >> id;
		stream >> spark::io::null_terminated(name);
		return stream? StreamResult::success : StreamResult::stream_error;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(le_stream auto& stream) const try {
		stream << id;
		stream << spark::io::null_terminated(name);
		return stream? StreamResult::success : StreamResult::stream_error;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // client, protocol, ember
