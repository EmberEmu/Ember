/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginSession.h"
#include "LoginHandlerBuilder.h"
#include <shared/threading/ThreadPool.h>
#include <memory>

namespace ember {

LoginSession::LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	                       log::Logger* logger, ThreadPool& pool, const LoginHandlerBuilder& builder)
						   : handler_(builder.create(*this)), logger_(logger), pool_(pool),
                             NetworkSession(sessions, std::move(socket), logger) { }

void LoginSession::handle_packet(spark::Buffer& buffer) try {
	boost::optional<grunt::PacketHandle> packet = grunt_handler_.try_deserialise(buffer);

	if(packet) {
		LOG_DEBUG(logger_) << "Packet done" << LOG_ASYNC;	
	}

} catch(grunt::bad_packet& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	close_session();
}

void LoginSession::execute_async(std::shared_ptr<Action> action) {
	auto self(shared_from_this());

	pool_.run([action, this, self] {
		action->execute();
		strand().post([action, this, self] {
			async_completion(action);
		});
	});
}

void LoginSession::async_completion(std::shared_ptr<Action> action) try {
	if(!handler_.update_state(action)) {
		// todo
	}
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	// todo
}

} // ember