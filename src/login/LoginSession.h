/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginHandler.h"
#include "NetworkSession.h"
#include "SocketType.h"
#include "StreamTypes.h"
#include "grunt/Packet.h"
#include "grunt/Handler.h"
#include <logger/LoggerFwd.h>
#include <thread/ThreadPool.h>
#include <memory>

namespace ember {

class LoginHandlerBuilder;

class LoginSession final : public NetworkSession<LoginSession> {
	thread::ThreadPool& pool_;
	LoginHandler handler_;
	log::Logger& logger_;
	grunt::Handler grunt_handler_;

	void async_completion(Action& action);
	void write_packet(const grunt::Packet& packet, WriteCallback&& cb);
	void execute_async(std::unique_ptr<Action> action);

public:
	LoginSession(SessionManager& sessions,
	             tcp_strand_socket socket,
	             log::Logger& logger,
	             thread::ThreadPool& pool,
	             const LoginHandlerBuilder& builder);

	bool handle_packet(BufferType& buffer);
};

} // ember