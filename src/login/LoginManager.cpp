/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginManager.h"
#include "Actions.h"
#include <logger/Logger.h>
#include <shared/threading/ThreadPool.h>
#include <functional>
#include <utility>

namespace ember {

void LoginManager::action_complete(std::shared_ptr<Session> session, std::shared_ptr<Action> action) try {
	if(!session->handler.update_state(action)) {
		close_session(session);
	}
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	close_session(session);
}

void LoginManager::execute_action(std::shared_ptr<Session> session, std::shared_ptr<Action> action) {
	pool_.run([session, action, this]() {
		action->execute();
		session->strand.dispatch(std::bind(&LoginManager::action_complete, this, session, action));
	});
}

void LoginManager::write(std::shared_ptr<Session> session, std::shared_ptr<Packet> packet) {
	session->timer.async_wait(std::bind(&LoginManager::timeout, this, session, std::placeholders::_1));

	session->socket.async_send(boost::asio::buffer(*packet),
		session->strand.wrap(create_alloc_handler(allocator_,
		[this, packet, session](boost::system::error_code ec, std::size_t) {
			if(!ec) {
				session->timer.cancel();
			} else {
				LOG_DEBUG(logger_) << "On send: " << ec.message() << LOG_ASYNC;
				close_session(session);
			}
		}
	)));
}

void LoginManager::read(std::shared_ptr<Session> session) {
	session->timer.async_wait(std::bind(&LoginManager::timeout, this, session, std::placeholders::_1));
	
	auto& buffer = session->buffer;

	session->socket.async_receive(boost::asio::buffer(buffer.store(), buffer.free()),
		session->strand.wrap(create_alloc_handler(allocator_,
		[this, session](boost::system::error_code ec, std::size_t size) {
			if(!ec) {
				session->timer.cancel();
				session->buffer.advance(size);
				handle_packet(session);
			} else {
				close_session(session);
			}
		}
	)));
}

void LoginManager::accept_connection() {
	acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
		if(ec) {
			return;
		}

		auto ip = socket_.remote_endpoint().address();

		if(ban_list_.is_banned(ip)) {
			LOG_DEBUG(logger_) << "Rejected connection from banned IP range" << LOG_ASYNC;
			return;
		}

		LOG_DEBUG(logger_) << "Accepted connection " << ip.to_string() << ":"
		                   << socket_.remote_endpoint().port() << LOG_ASYNC;

		start_session(std::move(socket_));
		accept_connection();
	});
}

void LoginManager::start_session(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto& service = socket.get_io_service();
	auto address = socket.remote_endpoint().address().to_string();

	auto session = std::make_shared<Session>(std::move(create_handler_(address)),
	                                         std::move(socket), service);

	session->handler.on_action = std::bind(&LoginManager::execute_action, this, session, std::placeholders::_1);
	session->handler.on_send = std::bind(&LoginManager::write, this, session, std::placeholders::_1);
	session->timer.expires_from_now(boost::posix_time::seconds(SOCKET_ACTIVITY_TIMEOUT));
	read(session);
}

void LoginManager::handle_packet(std::shared_ptr<Session> session) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(check_packet_completion(session->buffer)) {
		if(!session->handler.update_state(session->buffer)) {
			close_session(session);
		}

		session->buffer.clear();
	}

	read(session);
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	close_session(session);
}

void LoginManager::close_session(std::shared_ptr<Session> session) {
	boost::system::error_code ec;
	session->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	session->socket.close();
}

void LoginManager::timeout(std::shared_ptr<Session> session, const boost::system::error_code& ec) {
	if(!ec) {
		close_session(session);
	}
}

bool LoginManager::check_packet_completion(const PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(buffer.size() < sizeof(protocol::ClientOpcodes)) {
		return false;
	}

	auto opcode = *static_cast<const protocol::ClientOpcodes*>(buffer.data());

	switch(opcode) {
		case protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE:
		case protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE:
			return check_challenge_completion(buffer);
		case protocol::ClientOpcodes::CMSG_LOGIN_PROOF:
			return buffer.size() >= sizeof(protocol::ClientLoginProof);
		case protocol::ClientOpcodes::CMSG_RECONNECT_PROOF:
			return buffer.size() >= sizeof(protocol::ClientReconnectProof);
		case protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST:
			return buffer.size() >= sizeof(protocol::RequestRealmList);
		default:
			throw std::runtime_error("Unhandled opcode");
	}
}

bool LoginManager::check_challenge_completion(const PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto data = buffer.data<const protocol::ClientLoginChallenge>();

	// Ensure we've at least read the challenge header before continuing
	if(buffer.size() < sizeof(protocol::ClientLoginChallenge::Header)) {
		return false;
	}

	// Ensure we've read the entire packet
	std::size_t completed_size = data->header.size + sizeof(data->header);

	if(buffer.size() < completed_size) {
		std::size_t remaining = completed_size - buffer.size();
		
		// Continue reading if there's enough space in the buffer
		if(remaining < buffer.free()) {
			return false;
		}

		// Client claimed the packet is bigger than it ought to be
		throw std::runtime_error("Buffer too small to hold challenge packet");
	}

	// Packet should be complete by this point
	auto packet = buffer.data<const protocol::ClientLoginChallenge>();

	if(buffer.size() < sizeof(protocol::ClientLoginChallenge)
		|| packet->username + packet->username_len != buffer.data<const char>() + buffer.size()) {
		throw std::runtime_error("Malformed challenge packet");
	}

	return true;
}

} // ember