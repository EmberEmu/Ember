/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <spark/buffers/Shared.h>
#include <shared/utility/UTF8String.h>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>


namespace ember::protocol::server {

namespace be = boost::endian;

struct CharacterRename final {
	Result result;
	be::little_uint64_t id;
	utf8_string name;
	
	StreamResult read_from_stream(auto& stream) try {
		stream >> result;

		if(result == protocol::Result::response_success) {
			stream >> id;
			stream >> spark::io::null_terminated(name);
		}

		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << result;

		if(result == protocol::Result::response_success) {
			stream << id;
			stream << spark::io::null_terminated(name);
		}

		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
