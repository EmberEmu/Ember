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
#include "SessionManager.h"
#include "SocketType.h"
#include <logger/Logger.h>
#include <shared/ClientIdent.h>

namespace ember::realm {

class Client {
	ClientHandler handler_;
	ClientConnection connection_;

public:
	Client(SessionManager& sessions, tcp_socket socket, ClientIdent ident, log::Logger& logger)
		: handler_(ident, socket.get_executor(), logger) 
		, connection_(sessions, std::move(socket), logger) {
		handler_.set_connection(&connection_);
		connection_.set_handler(&handler_);
	}

	void start() {
		connection_.start();
		handler_.start();
	}

	ClientConnection& connection() {
		return connection_;
	}

	const ClientConnection& connection() const {
		return connection_;
	}

	ClientHandler& handler() {
		return handler_;
	}

	const ClientHandler& handler() const {
		return handler_;
	}
};

} // realm, ember
