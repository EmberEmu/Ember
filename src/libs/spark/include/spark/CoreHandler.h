/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Link.h>
#include <spark/temp/MessageRoot_generated.h>
#include <logger/Logging.h>
#include <chrono>

namespace ember { namespace spark {

class Service;

class CoreHandler {
	const Service* service_;
	log::Logger* logger_;
	log::Filter filter_;

	void handle_ping(const Link& link, const messaging::MessageRoot* message);
	void handle_pong(const Link& link, const messaging::MessageRoot* message);
	void send_ping(const Link& link);
	void send_pong(const Link& link, std::uint64_t time);

public:
	CoreHandler(const Service* service, log::Logger* logger, log::Filter filter);
	void handle_message(const Link& link, const messaging::MessageRoot* message);
	void handle_event(const Link& link, LinkState state);
};

}}