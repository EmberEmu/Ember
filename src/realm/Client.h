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
	/*
	 * We're referencing builders rather than moving the objects in because they're non-movable types
	 * and making them safely movable under *all* conditions isn't worth the additional complexity and
	 * performance penalties we'd have to pay - fast event dispatching would no longer play nicely with
	 * RPC and we don't want to allocate the objects individually.
	 * 
	 * They'd safe to move as long as it's done before calling start but it's best not to add the ability
	 * at all unless it's always safe to do so.
 	 */
	Client(const ClientHandlerBuilder& ch_builder, const ClientConnectionBuilder& cc_builder,
	       tcp_socket socket, std::size_t index)
		: handler_(ch_builder.create(index, socket.get_executor()))
		, connection_(cc_builder.create(std::move(socket))) {}

	~Client() {
		stop();
	}

	void start() {
		// Referencing each other like this is fine because they won't start running until we return
		// - that's assuming we're running on the same Asio worker, which we really should be
		handler_.start(connection_);
		connection_.start(handler_);
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