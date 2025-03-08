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

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream >> id;
			stream >> name;
		}

		return StreamResult::SUCCESS;
	} catch(const std::exception&) {
		return StreamResult::FAILED;
	}

	StreamResult write_to_stream(auto& stream) const {
		stream << result;

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream << id;
			stream << name;
		}

		return StreamResult::SUCCESS;
	}
};

} // server, protocol, ember
