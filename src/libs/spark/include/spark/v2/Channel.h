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
#include <spark/v2/Link.h>
#include <memory>
#include <span>
#include <cstdint>

namespace ember::spark::v2 {

class RemotePeer;

class Channel final : std::enable_shared_from_this<Channel> {
public:
	enum class State {
		AWAITING, OPEN
	};

private:
	State state_ = State::AWAITING;
	Handler* handler_ = nullptr;
	std::string service_;
	std::string banner_;

	Link link_{};
	std::uint8_t id_ = 0;

	void link_up();

public:
	Channel(std::uint8_t id, std::string banner, std::string service, 
	        Handler* handler, std::weak_ptr<RemotePeer> net);
	Channel() = default;
	~Channel();

	Handler* handler();
	State state();
	void open();
	bool is_open();
	void dispatch(const MessageHeader& header, std::span<const std::uint8_t> data);
	void send();
};

} // v2, spark, ember