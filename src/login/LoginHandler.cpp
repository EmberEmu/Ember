/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include <shared/misc/PacketStream.h>
#include <shared/memory/ASIOAllocator.h>
#include <logger/Logging.h>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <vector>
#include <thread>

namespace ember {

void LoginHandler::start() {
	read();
}

void LoginHandler::process_login_challenge() {
	auto packet = buffer_.data<protocol::ClientLoginChallenge>();

	if(buffer_.size() < sizeof(protocol::ClientLoginChallenge)
		|| packet->username + packet->username_len != buffer_.data<char>() + buffer_.size()) {
		LOG_DEBUG(logger_) << "Malformed login challenge packet" << LOG_FLUSH;
		socket_.close();
		return;
	}

	//The username in the packet isn't null-terminated so don't try using it directly
	std::string user(packet->username, packet->username_len);
	GameVersion version{packet->major, packet->minor, packet->patch, packet->build};

	LOG_DEBUG(logger_) << "Challenge: " << user << ", " << version << ", "
	                   << socket_.remote_endpoint().address().to_string() << LOG_FLUSH;

	//Should probably have a patcher to handle this
	Authenticator::PATCH_STATE patch_level = auth_.verify_client_version(version);

	if(patch_level != Authenticator::PATCH_STATE::OK) {
		if(patch_level == Authenticator::PATCH_STATE::TOO_OLD) {
			//stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_VERSION_UPDATE;
			//write(packet);
		} else {
			LOG_DEBUG(logger_) << "Rejecting client version " << version << LOG_FLUSH;
			auto packet = std::allocate_shared<Packet>(boost::fast_pool_allocator<Packet>());
			PacketStream<Packet> stream(*packet);
			stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_VERSION_INVALID;
			write(packet);
		}
	}

	auto account_status = auth_.check_account(user);
	
	if(account_status == ember::Authenticator::ACCOUNT_STATUS::DAL_ERROR) {
		LOG_ERROR(logger_) << "DAL failure "  << LOG_FLUSH;
		auto packet = std::allocate_shared<Packet>(boost::fast_pool_allocator<Packet>());
		PacketStream<Packet> stream(*packet);
		stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_DB_BUSY;
		write(packet);
	} else if(account_status == ember::Authenticator::ACCOUNT_STATUS::NOT_FOUND) {
		LOG_DEBUG(logger_) << "Account not found " << LOG_FLUSH;
		auto packet = std::make_shared<Packet>();
		PacketStream<Packet> stream(*packet);
		stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_UNKNOWN_ACCOUNT;
		write(packet);
	}

	buffer_.clear();
	read();
}

void LoginHandler::login_challenge_read() {
	auto data = buffer_.data<protocol::ClientLoginChallenge>();

	//Ensure we've at least read the packet header before continuing
	if(buffer_.size() < sizeof(protocol::ClientLoginChallenge::Header)) {
		read();
		return;
	}

	//Ensure we've read the entire packet
	std::size_t completed_size = data->header.size + sizeof(data->header);

	if(buffer_.size() < completed_size) {
		std::size_t remaining = completed_size - buffer_.size();
		
		if(remaining < buffer_.free()) {
			read();
		}

		return;
	}

	//Packet should be fine by this point
	process_login_challenge();
}

void LoginHandler::handle_client_proof() {
	auto data = buffer_.data<protocol::ClientLoginProof>();

	if(data->opcode != protocol::CMSG_OPCODE::CMSG_LOGIN_PROOF) {
		LOG_DEBUG(logger_) << "Invalid opcode found - expected login proof" << LOG_FLUSH;
		return;
	}

	LOG_DEBUG(logger_) << "Found login proof" << LOG_FLUSH;
}

void LoginHandler::handle_packet() {
	if(buffer_.size() < sizeof(protocol::CMSG_OPCODE)) {
		read();
	}

	auto opcode = *static_cast<protocol::CMSG_OPCODE*>(buffer_.data());

	switch(opcode) {
		case protocol::CMSG_OPCODE::CMSG_LOGIN_CHALLENGE:
			login_challenge_read();
			break;
		case protocol::CMSG_OPCODE::CMSG_RECONNECT_CHALLENGE:
			LOG_DEBUG(logger_) << "Unhandled CMSG_RECONNECT_CHALLENGE" << LOG_FLUSH;
			break;
		case protocol::CMSG_OPCODE::CMSG_LOGIN_PROOF:
			LOG_DEBUG(logger_) << "Unhandled CMSG_LOGIN_PROOF" << LOG_FLUSH;
			break;
		case protocol::CMSG_OPCODE::CMSG_RECONNECT_PROOF:
			LOG_DEBUG(logger_) << "Unhandled CMSG_RECONNECT_PROOF" << LOG_FLUSH;
			break;
		case protocol::CMSG_OPCODE::CMSG_REQUEST_REALM__LIST:
			LOG_DEBUG(logger_) << "Unhandled CMSG_RECONNECT_PROOF" << LOG_FLUSH;
			break;
		default:
			LOG_DEBUG(logger_) << "Unhandled packet type" << LOG_FLUSH;
			break;
	}
}

void LoginHandler::read() {
	auto self = shared_from_this();

	timer_.expires_from_now(boost::posix_time::seconds(5));
	timer_.async_wait(std::bind(&LoginHandler::close, self, std::placeholders::_1));
	
	socket_.async_receive(boost::asio::buffer(buffer_.store(), buffer_.free()),
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			if(!ec) {
				timer_.cancel();
				buffer_.advance(size);
				handle_packet();
			}
		}
	)));
}

void LoginHandler::write(std::shared_ptr<Packet> buffer) {
	auto self = shared_from_this();

	boost::asio::async_write(socket_, boost::asio::buffer(*buffer),
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self, buffer](boost::system::error_code ec, std::size_t) {
			if(ec) {
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

} //ember