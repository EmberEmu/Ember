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
                 Handler* handler, std::weak_ptr<RemotePeer> net)
	: link_{.banner = "", .net = net, .channel_id = id}, // temp, I think
	  banner_(banner),
	  service_(service),
	  handler_(handler) {
	/*if(state == State::OPEN) {
		link_up();
	}*/
}

void Channel::open() {
	// multiple open calls should have no effect
	if(state_ != State::OPEN) {
		link_up();
	}

	state_ = State::OPEN;
}

bool Channel::is_open() {
	return state_ == State::OPEN;
}

void Channel::dispatch(const MessageHeader& header, std::span<const std::uint8_t> data) {
	// draw the rest of the owl
}

void Channel::send() {
	// todo
}

auto Channel::state() -> State {
	return state_;
}

Handler* Channel::handler() {
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

	handler_->on_link_down(link_);
}

} // v2, spark, ember