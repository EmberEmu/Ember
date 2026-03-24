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
#include "SocketType.h"
#include <logger/Logger.h>
#include <shared/ClientIdent.h>
#include <functional>

namespace ember::realm {

class Client {
public:
	using OnClose = std::function<void()>;

private:
	ClientHandler handler_;
	ClientConnection connection_;

public:
	Client(tcp_socket socket, ClientIdent ident, log::Logger& logger)
		: handler_(std::move(ident), socket.get_executor(), logger) 
		, connection_(std::move(socket), logger) {
		handler_.set_connection(&connection_);
		connection_.set_handler(&handler_);
	}

	void start() {
		connection_.start();
		handler_.start();
	}

	void stop() {
		handler_.stop();
		connection_.stop();
	}

	void on_close(OnClose on_close) {
		connection_.set_on_disconnect([&, on_close]() {
			handler_.stop();
			on_close();
		});
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

using ClientPtr = std::unique_ptr<Client, std::function<void(Client*)>>;

} // realm, ember
