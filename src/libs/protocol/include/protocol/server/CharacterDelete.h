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
#include <stdexcept>

namespace ember::protocol::server {

struct CharacterDelete final {
	Result result;
	
	StreamResult read_from_stream(auto& stream) try {
		stream >> result;
		return StreamResult::SUCCESS;
	} catch(const std::exception&) {
		return StreamResult::FAILED;
	}

	StreamResult write_to_stream(auto& stream) const {
		stream << result;
		return StreamResult::SUCCESS;
	}
};

} // server, protocol, ember
