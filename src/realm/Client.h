/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"
#include "ClientHandler.h"
#include "ClientIdent.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <chrono>

namespace ember::realm {

class Client {
	const std::chrono::steady_clock::time_point start_time_;

	ClientHandler handler_;
	ClientConnection connection_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;
	bool running_;

	void start();

	bool handle_self_event(const Event& event);
	void handle_kick();
	void handle_request_stop();
	void packet_log_start();

public:
	Client(ClientHandler handler, ClientConnection connection,
	       EventDispatcher& dispatcher, log::Logger& logger);
	~Client();

	Client(Client&) = delete;
	Client(Client&&) = delete;
	Client& operator=(Client&) = delete;
	Client& operator=(Client&&) = delete;

	void stop();
	bool stopped() const;

	void handle_event(const Event& event);
	std::chrono::steady_clock::time_point start_time() const;

	const ClientIdent& ident() const;
	const ClientConnection& connection() const;
	const ClientHandler& handler() const;
};

} // realm, ember