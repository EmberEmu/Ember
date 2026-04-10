/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnectionBuilder.h"
#include "ClientHandlerBuilder.h"
#include "SocketType.h"
#include <logger/Logger.h>
#include <functional>

namespace ember::realm {

class Client {
public:
	using OnClose = std::function<void()>;

private:
	ClientHandler handler_;
	ClientConnection connection_;

public:
	Client(ClientHandlerBuilder ch_builder, ClientConnectionBuilder cc_builder, tcp_socket socket, std::size_t index)
		: handler_(ch_builder.create(index, socket.get_executor()))
		, connection_(cc_builder.create(std::move(socket))) {
		handler_.set_connection(&connection_);
		connection_.set_handler(&handler_);
	}

	~Client() {
		stop();
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
		connection_.set_on_disconnect([&, fn = std::move(on_close)]() {
			handler_.stop();
			fn();
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

} // realm, ember
