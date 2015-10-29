/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginSession.h"
#include "LoginHandlerBuilder.h"
#include <shared/threading/ThreadPool.h>
#include <memory>
#include <shared/util/FormatPacket.h>

namespace ember {

LoginSession::LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
                           log::Logger* logger, ThreadPool& pool, const LoginHandlerBuilder& builder)
                           : handler_(builder.create(*this, remote_address() + ":" + std::to_string(remote_port()))),
                             logger_(logger), pool_(pool),
                             NetworkSession(sessions, std::move(socket), logger) {
	handler_.send = std::bind(&LoginSession::write_chain, this, std::placeholders::_1);
	handler_.execute_async = std::bind(&LoginSession::execute_async, this, std::placeholders::_1);
}

bool LoginSession::handle_packet(spark::Buffer& buffer) try {
	boost::optional<grunt::PacketHandle> packet = grunt_handler_.try_deserialise(buffer);

	if(packet) {
		handler_.update_state(packet->get());
	}

	return true;
} catch(grunt::bad_packet& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	return false;
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
		close_session(); // todo change
	}
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	close_session();
}

void LoginSession::write_chain(std::shared_ptr<grunt::Packet> packet) { // todo - change to unique_ptr in VS2015 (binding bug)
	auto chain = std::make_shared<spark::BufferChain<1024>>();
	spark::BinaryStream stream(*chain);
	packet->write_to_stream(stream);
	NetworkSession::write_chain(chain);
}

} // ember