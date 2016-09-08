/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include <game_protocol/client/CMSG_AUTH_SESSION.h>

namespace ember {

struct QueuePosition : public Event {
	QueuePosition(std::size_t position) : Event { EventType::QUEUE_UPDATE_POSITION }, position(position) { }

	std::size_t position;
};

struct QueueSuccess : public Event {
	QueueSuccess(protocol::CMSG_AUTH_SESSION packet)
	             : Event { EventType::QUEUE_SUCCESS },
	               packet(std::move(packet)) { }

	protocol::CMSG_AUTH_SESSION packet;
};

} // ember