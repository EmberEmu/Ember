/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <vector>
#include <cstdint>

namespace ember { namespace spark {

class MessageHandler {

	enum class State {
		HANDSHAKING, NEGOTIATING, FORWARDING
	} state_ = State::HANDSHAKING;

	log::Logger* logger_;
	log::Filter filter_;

public:
	MessageHandler(log::Logger* logger, log::Filter filter);
	bool handle_message(const std::vector<std::uint8_t>& net_buffer);
};

}} // spark, ember