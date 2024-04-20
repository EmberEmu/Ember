/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Channel.h>
#include <cassert>

namespace ember::spark::v2 {

Channel::Channel(std::uint8_t id, std::string banner, std::string service,
                 Handler* handler, std::shared_ptr<Connection> net)
	: link_{.peer_banner = banner, .service_name = service, .net = weak_from_this()},
	  handler_(handler),
	  channel_id_(id) {}

void Channel::open() {
	if(state_ != State::OPEN) {
		state_ = State::OPEN;
		link_up();
	}
}

bool Channel::is_open() const {
	return state_ == State::OPEN;
}

void Channel::dispatch(const MessageHeader& header, std::span<const std::uint8_t> data) {
	// draw the rest of the owl
}

void Channel::send() {
	// todo
}

auto Channel::state() const -> State {
	return state_;
}

Handler* Channel::handler() const {
	return handler_;
}

void Channel::link_up() {
	assert(handler_);
	handler_->on_link_up(link_);
}

Channel::~Channel() {
	if(!handler_) {
		return;
	}

	if(is_open()) {
		handler_->on_link_down(link_);
	}
}

} // v2, spark, ember