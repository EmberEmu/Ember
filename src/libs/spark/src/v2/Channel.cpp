/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Channel.h>

namespace ember::spark::v2 {

void Channel::message(const MessageHeader& header, std::span<const std::uint8_t> data) {
	// draw the rest of the owl
}

auto Channel::state() -> State {
	return state_;
}

void Channel::handler(Handler* handler) {
	handler_ = handler;
}

Handler* Channel::handler() {
	return handler_;
}

void Channel::state(State state) {
	state_ = state;
}

void Channel::reset() {
	handler_ = nullptr;
	state_ = State::EMPTY;
}

} // v2, spark, ember