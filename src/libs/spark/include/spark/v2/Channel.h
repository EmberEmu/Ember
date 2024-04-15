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
#include <cstdint>

namespace ember::spark::v2 {

class RemotePeer;

class Channel {
public:
	enum class State {
		EMPTY, HALF_OPEN, OPEN
	};

private:
	State state_ = State::EMPTY;
	Handler* handler_ = nullptr;

	Link link_{};
	std::uint8_t id_ = 0;

	void link_up();

public:
	Channel(std::uint8_t id, State state, Handler* handler, std::weak_ptr<RemotePeer> net);
	Channel() = default;
	~Channel();

	Handler* handler();
	State state();
	void state(State state);
	void message(const MessageHeader& header, std::span<const std::uint8_t> data);
};

} // v2, spark, ember