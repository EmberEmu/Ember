/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include <shared/memory/ASIOAllocator.h>
#include <logger/Logging.h>
#include <vector>
#include <thread>

namespace ember {

void LoginHandler::start() {
	LOG_DEBUG(logger_) << "Initial state: " << (int)state_ << LOG_FLUSH;
	read(sizeof(opcodes::ClientLoginChallenge::Header));
}

void LoginHandler::process_login_challenge() {
	auto packet = buffer_.data<opcodes::ClientLoginChallenge>();

	if(buffer_.size() < sizeof(opcodes::ClientLoginChallenge)
		|| packet->username + packet->username_len != buffer_.data<char>() + buffer_.size()) {
		LOG_DEBUG(logger_) << "Bad login challenge packet" << LOG_FLUSH;
		return;
	}

	/*;
	
	if(std::find(versions_.begin(), versions_.end(), version) == versions_.end()) {
		LOG_DEBUG(logger_) << "Unhandled client version: " << version << LOG_FLUSH;
	}*/
	GameVersion version{ packet->major, packet->minor, packet->patch, packet->build };
	auth_.verify_client_version(version);

	read(sizeof(opcodes::ClientLoginProof));
}

void LoginHandler::login_challenge_read() {
	auto data = buffer_.data<opcodes::ClientLoginChallenge>();

	if(data->header.opcode != opcodes::CLIENT::CMSG_LOGIN_CHALLENGE) {
		LOG_DEBUG(logger_) << "Invalid opcode found - expected login challenge" << LOG_FLUSH;
		return;
	}

	if(buffer_.size() != data->header.size + sizeof(data->header)) {
		if(data->header.size < buffer_.free()) {
			read(data->header.size);
		}
	} else {
		process_login_challenge();
	}
}

void LoginHandler::handle_client_proof() {
	auto data = buffer_.data<opcodes::ClientLoginProof>();

	if(data->opcode != opcodes::CLIENT::CMSG_LOGIN_PROOF) {
		LOG_DEBUG(logger_) << "Invalid opcode found - expected login proof" << LOG_FLUSH;
		return;
	}

	LOG_DEBUG(logger_) << "Found login proof" << LOG_FLUSH;
}

void LoginHandler::handle_packet() {
	switch(state_) {
		case opcodes::CLIENT::CMSG_LOGIN_CHALLENGE:
			login_challenge_read();
			break;
		case opcodes::CLIENT::CMSG_LOGIN_PROOF:
			handle_client_proof();
			break;
		case opcodes::CLIENT::CMSG_REQUEST_REALM__LIST:
			break;
	}
}

void LoginHandler::read(std::size_t read) {
	auto self = shared_from_this();

	timer_.expires_from_now(boost::posix_time::seconds(15));
	timer_.async_wait(std::bind(&LoginHandler::close, self, std::placeholders::_1));
	
	boost::asio::async_read(socket_, boost::asio::buffer(buffer_.store(), read),
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			if(!ec) {
				timer_.cancel();
				buffer_.advance(size);
				handle_packet();
			} else {
				LOG_DEBUG(logger_) << ec.message() << LOG_FLUSH;
			}
		}
	)));
}

void LoginHandler::write(std::shared_ptr<std::vector<char>> buffer) {
	auto self = shared_from_this();

	boost::asio::async_write(socket_, boost::asio::buffer(*buffer),
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self, buffer](boost::system::error_code ec, std::size_t size) {
			if(!ec) {
				LOG_DEBUG(logger_) << ec.message() << LOG_FLUSH;
			}
		}
	)));
}

void LoginHandler::close(const boost::system::error_code& error) {
	if(!error) {
		socket_.close();
	}
}

LoginHandler::~LoginHandler() {
	if(!socket_.is_open()) {
		return;
	}
}

} //ember