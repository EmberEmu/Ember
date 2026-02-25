/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginSession.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <thread/ThreadPool.h>
#include <memory>
#include <utility>

namespace ember {

namespace bai = boost::asio::ip;

class LoginHandlerBuilder;
class SessionManager;

class NetworkSessionBuilder {
public:
	virtual std::shared_ptr<LoginSession> create(SessionManager& sessions,
												 tcp_strand_socket socket,
	                                             log::Logger& logger) const = 0;

	virtual ~NetworkSessionBuilder() = default;
};

class LoginSessionBuilder final : public NetworkSessionBuilder {
	const LoginHandlerBuilder& builder_;
	thread::ThreadPool& pool_;

public:
	LoginSessionBuilder(const LoginHandlerBuilder& builder, thread::ThreadPool& pool)
		: builder_(builder), pool_(pool) { }

	std::shared_ptr<LoginSession> create(SessionManager& sessions,
										 tcp_strand_socket socket,
	                                     log::Logger& logger) const override {
		return std::make_shared<LoginSession>(sessions, std::move(socket), logger, pool_, builder_);
	}
};

} // ember