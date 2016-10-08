/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Link.h>
#include <spark/ServicesMap.h>
#include <logger/Logging.h>
#include <set>
#include <cstdint>

namespace ember { namespace spark {

class NetworkSession;
class EventDispatcher;
class LinkMap;
struct Message;

class MessageHandler {
	enum class State {
		HANDSHAKING, NEGOTIATING, DISPATCHING
	} state_ = State::HANDSHAKING;

	Link peer_;
	const Link& self_;
	const EventDispatcher& dispatcher_;
	ServicesMap& services_;
	log::Logger* logger_;
	std::set<std::int32_t> matches_;
	bool initiator_;

	void dispatch_message(const Message& message);
	bool negotiate_protocols(NetworkSession& net, const Message& message);
	bool establish_link(NetworkSession& net, const Message& message);
	void send_banner(NetworkSession& net);
	void send_negotiation(NetworkSession& net);

public:
	MessageHandler(const EventDispatcher& dispatcher, ServicesMap& services, const Link& link,
	               bool initiator, log::Logger* logger);
	~MessageHandler();

	bool handle_message(NetworkSession& net, const messaging::core::Header* header,
	                    const std::uint8_t* data, std::uint32_t size);
	void start(NetworkSession& net);
};

}} // spark, ember