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

struct NameQueryResponse final {
	std::uint64_t guid;
	std::string name;
	std::string realm;
	std::uint32_t race;
	std::uint32_t gender;
	std::uint32_t _class;

	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> guid;
		stream >> spark::io::null_terminated(name);
		stream >> spark::io::null_terminated(realm);
		stream >> race;
		stream >> gender;
		stream >> _class;
		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << guid;
		stream << spark::io::null_terminated(name);
		stream << spark::io::null_terminated(realm);
		stream << race;
		stream << gender;
		stream << _class;
		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember
