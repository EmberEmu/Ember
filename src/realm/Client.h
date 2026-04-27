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
#include "Forwards.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <shared/ClientIdent.h>
#include <atomic>

namespace ember::realm {

class Client {
	inline static std::atomic_size_t curr_clients_;
	inline static std::atomic_size_t peak_clients_;

	ClientIdent ident_;
	ClientHandler handler_;
	ClientConnection connection_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;
	std::atomic_bool running_;

	bool handle_self_event(const Event& event);
	void handle_kick();
	void handle_request_stop();
	void packet_log_start();
	void update_peak();

public:
	/*
	 * We're referencing builders rather than moving the objects in because they're non-movable types
	 * and making them safely movable under *all* conditions isn't worth the additional complexity and
	 * performance penalties we'd have to pay - fast event dispatching would no longer play nicely with
	 * RPC and we don't want to allocate the objects individually.
	 * 
	 * They'd be safe to move as long as it's done before calling start but it's best not to add the ability
	 * at all unless it's always safe to do so. I did it and promptly undid it. Trust me, bro.
 	 */
	Client(tcp_socket socket, std::size_t index, EventDispatcher& dispatcher, log::Logger& logger,
	       const ClientHandlerBuilder& ch_builder, const ClientConnectionBuilder& cc_builder);
	~Client();

	Client(Client&) = delete;
	Client(Client&&) = delete;
	Client& operator=(Client&) = delete;
	Client& operator=(Client&&) = delete;

	void start();
	void stop();
	bool stopped() const;

	void handle_event(const Event& event);

	const ClientIdent& uuid() const {
		return ident_;
	}

	const ClientConnection& connection() const;
	const ClientHandler& handler() const;

	static std::size_t curr_clients();
	static std::size_t peak_clients();
	static bool reserve_client_slot(std::size_t limit);
	static void free_client_slot();
};

} // realm, ember