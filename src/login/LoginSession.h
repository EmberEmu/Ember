/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "NetworkSession.h"
#include "LoginHandler.h"
#include "grunt/Handler.h"
#include <spark/Buffer.h>
#include <spark/BufferChain.h>
#include <logger/Logging.h>
#include <shared/threading/ThreadPool.h>
#include <memory>

namespace ember {

class LoginHandlerBuilder;
class ThreadPool;
class Metrics;

class LoginSession final : public NetworkSession {
	void async_completion(std::shared_ptr<Action> action);
public:
	ThreadPool& pool_;
	LoginHandler handler_;
	log::Logger* logger_;
	grunt::Handler grunt_handler_;

	LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	             log::Logger* logger, ThreadPool& pool, const LoginHandlerBuilder& builder);

	bool handle_packet(spark::Buffer& buffer) override;
	void execute_async(std::shared_ptr<Action> action);
	void write_chain(std::shared_ptr<grunt::Packet> packet); // todo - see todo on the definition
};

} // ember