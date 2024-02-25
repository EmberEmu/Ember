/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginSession.h"
#include "LoginHandlerBuilder.h"
#include "FilterTypes.h"
#include <shared/metrics/Metrics.h>
#include <shared/threading/ThreadPool.h>
#include <boost/asio/post.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <utility>

namespace ember {

LoginSession::LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
                           boost::asio::ip::tcp::endpoint ep, log::Logger* logger,
                           ThreadPool& pool, const LoginHandlerBuilder& builder)
                           : handler_(builder.create(remote_address())),
                             logger_(logger), pool_(pool), grunt_handler_(logger),
                             NetworkSession(sessions, std::move(socket), std::move(ep), logger) {
	handler_.send = [&](auto& packet) {
		write_chain(packet, false);
	};

	handler_.send_chunk = [&](auto& packet) {
		write_chain(packet, true);
	};

	handler_.execute_async = [&](auto action) {
		execute_async(action);
	};
}

bool LoginSession::handle_packet(spark::Buffer& buffer) try {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	auto packet = grunt_handler_.try_deserialise(buffer);

	if(packet) {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " -> "
			<< grunt::to_string(packet->opcode) << LOG_ASYNC;
		return handler_.update_state(*packet);
	}

	return true;
} catch(grunt::bad_packet& e) {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK) << e.what() << LOG_ASYNC;
	return false;
}

void LoginSession::execute_async(const std::shared_ptr<Action>& action) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	auto self(shared_from_this());

	pool_.run([action, this, self] {
		action->execute();

		boost::asio::post(get_executor(), [action, this, self] {
			async_completion(action);
		});
	});
}

void LoginSession::async_completion(const std::shared_ptr<Action>& action) try {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	if(!handler_.update_state(*action.get())) {
		close_session(); // todo change
	}
} catch(const std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	close_session();
}

// todo use a single chain here and create a write queue instead
void LoginSession::write_chain(const grunt::Packet& packet, bool notify) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " <- "
		<< grunt::to_string(packet.opcode) << LOG_ASYNC;

	auto chain = std::make_shared<spark::DynamicBuffer<1024>>();
	spark::BinaryStream stream(*chain);
	packet.write_to_stream(stream);
	NetworkSession::write_chain(chain, notify);
}

void LoginSession::on_write_complete() {
	handler_.on_chunk_complete();
}

} // ember