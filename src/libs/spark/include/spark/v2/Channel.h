/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Tracker.h>
#include <spark/v2/Handler.h>
#include <spark/v2/MessageHeader.h>

namespace ember::spark::v2 {

class Channel {
public:
	enum class State {
		EMPTY, HALF_OPEN, OPEN
	};

private:
	State state_ = State::EMPTY;
	Handler* handler_ = nullptr;

public:
	void handler(Handler* handler);
	Handler* handler();
	State state();
	void state(State state);
	void reset();
	void message(const MessageHeader& header, std::span<const std::uint8_t> data);
};

} // v2, spark, ember