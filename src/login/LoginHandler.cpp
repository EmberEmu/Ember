/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include <logger/Logging.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/assert.hpp>
#include <vector>
#include <thread>

namespace ember {

void LoginHandler::start() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	read();
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

	socket_.async_send(boost::asio::buffer(*buffer),
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self, buffer](boost::system::error_code ec, std::size_t) {
			if(!ec) {
				buffer_.clear();
				read();
			} else {
				LOG_DEBUG(logger_) << "On send: " << ec.message() << LOG_ASYNC;
			}
		}
	)));
}

void LoginHandler::close(const boost::system::error_code& ec) {
	if(!ec) {
		socket_.close();
	}
}

void LoginHandler::handle_packet() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(buffer_.size() < sizeof(protocol::ClientOpcodes)) {
		read();
	}

	auto opcode = *static_cast<protocol::ClientOpcodes*>(buffer_.data());

	if(initial_) {
		if(opcode == protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE
			|| opcode == protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE) {
			state_ = opcode;
			initial_ = false;
		} else {
			LOG_DEBUG(logger_) << "Invalid initial state, connection dropped" << LOG_ASYNC;
			return;
		}
	}

	if(opcode != state_) {
		LOG_DEBUG(logger_) << "States out of sync, connection dropped" << LOG_ASYNC;
		return;
	}

	if(opcode == protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE
	   || opcode == protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE) {
		read_challenge();
	} else if(opcode == protocol::ClientOpcodes::CMSG_LOGIN_PROOF) {
		if(buffer_.size() >= sizeof(protocol::ClientLoginProof)) {
			send_login_proof();
		} else {
			read();
		}
	} else if(opcode == protocol::ClientOpcodes::CMSG_RECONNECT_PROOF) {
		if(buffer_.size() >= sizeof(protocol::ClientReconnectProof)) {
			send_reconnect_proof();
		} else {
			read();
		}
	} else if(opcode == protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST) {
		LOG_DEBUG(logger_) << "Unhandled CMSG_REQUEST_REALM_LIST" << LOG_ASYNC;
		read();
	} else {
		LOG_DEBUG(logger_) << "Unhandled opcode  " << static_cast<int>(opcode) << LOG_ASYNC;
	}
}

void LoginHandler::read_challenge() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto data = buffer_.data<protocol::ClientLoginChallenge>();

	//Ensure we've at least read the challenge header before continuing
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

	//Packet should be complete by this point
	process_challenge();
}

void LoginHandler::process_challenge() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto packet = buffer_.data<protocol::ClientLoginChallenge>();
	auto opcode = packet->header.opcode;

	if(buffer_.size() < sizeof(protocol::ClientLoginChallenge)
		|| packet->username + packet->username_len != buffer_.data<char>() + buffer_.size()) {
		LOG_DEBUG(logger_) << "Malformed challenge packet" << LOG_ASYNC;
		socket_.close();
		return;
	}

	//The username in the packet isn't null-terminated so don't try using it directly
	username_ = std::string(packet->username, packet->username_len);
	GameVersion version{packet->major, packet->minor, packet->patch, packet->build};

	LOG_DEBUG(logger_) << "Challenge: " << username_ << ", " << version << ", "
	                   << socket_.remote_endpoint().address().to_string() << LOG_ASYNC;

	//Should probably have a patcher to handle this
	Authenticator::PatchState patch_level = auth_.verify_client_version(version);

	if(patch_level == Authenticator::PatchState::OK) {
		auto self = shared_from_this();
		
		async_.run([this, self, opcode]() {
			if(opcode == protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE) {
				auto status = auth_.check_account(username_);
				service_.dispatch(std::bind(&ember::LoginHandler::send_login_challenge, self, status));
			} else if(opcode == protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE) {
				auto reply = auth_.begin_reconnect(username_);
				service_.dispatch(std::bind(&ember::LoginHandler::send_reconnect_challenge, self, reply));
			}
		});
	} else if(patch_level == Authenticator::PatchState::TOO_OLD) {
		//patch
		//stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_VERSION_UPDATE;
		//write(packet);
	} else if(patch_level == Authenticator::PatchState::TOO_NEW) {
		LOG_DEBUG(logger_) << "Rejecting client version " << version << LOG_ASYNC;
		auto packet = std::allocate_shared<Packet>(boost::fast_pool_allocator<Packet>());
		PacketStream<Packet> stream(*packet);
		stream << protocol::ServerOpcodes::SMSG_LOGIN_CHALLENGE << std::uint8_t(0)
		       << protocol::ResultCodes::FAIL_VERSION_INVALID;
		initial_ = true;
		write(packet);
	} else {
		LOG_FATAL(logger_) << "Encountered an unknown version condition, "
		                   << static_cast<int>(patch_level) << LOG_SYNC;
		BOOST_ASSERT(false);
	}
}

void LoginHandler::build_login_challenge(PacketStream<Packet>& stream) {	
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto values = auth_.challenge_reply();

	//Server's public ephemeral key
	auto B = Botan::BigInt::encode_1363(values.B, protocol::PUB_KEY_LENGTH);
	std::reverse(B.begin(), B.end()); //Botan's buffers are big-endian, client is little-endian

	//Safe prime
	auto N = Botan::BigInt::encode_1363(values.gen.prime(), protocol::PRIME_LENGTH);
	std::reverse(N.begin(), N.end());
	
	///Salt
	auto salt = Botan::BigInt::encode(values.salt);
	std::reverse(salt.begin(), salt.end());
	
	//Do the stream writing after encoding the values so it's not in a bad state if there's an exception
	stream << protocol::ResultCodes::SUCCESS;
	stream << B;
	stream << std::uint8_t(values.gen.generator().bytes());
	stream << std::uint8_t(values.gen.generator().to_u32bit());
	stream << std::uint8_t(protocol::PRIME_LENGTH);
	stream << N << salt;
	stream << (Botan::AutoSeeded_RNG()).random_vec(16); //Random bytes, for some reason
	stream << std::uint8_t(0); //unknown
}

void LoginHandler::send_login_challenge(Authenticator::AccountStatus status) {	
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(*resp);

	stream << protocol::ServerOpcodes::SMSG_LOGIN_CHALLENGE << std::uint8_t(0);

	switch(status) {
		case ember::Authenticator::AccountStatus::OK:
			try {
				build_login_challenge(stream);
			} catch(std::exception& e) {
				LOG_ERROR(logger_) << e.what() << " thrown during encoding for " << username_ << LOG_ASYNC;
				stream << protocol::ResultCodes::FAIL_DB_BUSY;
			}
			break;
		case ember::Authenticator::AccountStatus::NOT_FOUND:
			//leaks information on whether the account exists (could send challenge anyway?)
			LOG_DEBUG(logger_) << "Account not found: " << username_ << LOG_ASYNC;
			stream << protocol::ResultCodes::FAIL_UNKNOWN_ACCOUNT;
			break;
		case ember::Authenticator::AccountStatus::DAL_ERROR:
			LOG_ERROR(logger_) << "DAL failure while retrieving details for " << username_ << LOG_ASYNC;
			stream << protocol::ResultCodes::FAIL_DB_BUSY;
			break;
		default:
			LOG_FATAL(logger_) << "Unhandled account state" << LOG_SYNC;
			BOOST_ASSERT(false);
	}

	state_ = protocol::ClientOpcodes::CMSG_LOGIN_PROOF;
	write(resp);
}

void LoginHandler::send_login_proof() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(buffer_.size() != sizeof(protocol::ClientLoginProof)) {
		LOG_DEBUG(logger_) << "Malformed login proof from " << username_ << LOG_ASYNC;
		return;
	}

	auto result = auth_.proof_check(buffer_.data<protocol::ClientLoginProof>());

	std::shared_ptr<Packet> resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(*resp);

	stream << protocol::ServerOpcodes::SMSG_LOGIN_PROOF;
	stream << result.first;

	if(result.first == protocol::ResultCodes::SUCCESS) {
		Botan::SecureVector<Botan::byte> m2 = Botan::BigInt::encode_1363(result.second, 20);
		std::reverse(m2.begin(), m2.end());
		stream << m2 << std::uint32_t(0); //proof << account flags

		auto self = shared_from_this();
		std::string ip = socket_.remote_endpoint().address().to_string();

		async_.run([this, self, resp, ip]() {
			try {
				auth_.set_logged_in(ip);
				auth_.set_session_key();
				state_ = protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST;
				service_.dispatch(std::bind(&ember::LoginHandler::write, self, resp));
				LOG_DEBUG(logger_) << username_ << " successfully authenticated" << LOG_ASYNC;
			} catch(std::exception& e) {
				LOG_ERROR(logger_) << "Unable to create session for " << username_ << ": "
				                   << e.what() << LOG_ASYNC;
			}
		});
	} else {
		initial_ = true;
		write(resp);
	}
}

void LoginHandler::send_reconnect_challenge(bool key_found) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!key_found) {
		return; 
	}

	auto rand = Botan::AutoSeeded_RNG().random_vec(16);
	auth_.set_reconnect_challenge(rand);

	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(*resp);

	stream << protocol::ServerOpcodes::SMSG_RECONNECT_CHALLENGE;
    stream << protocol::ResultCodes::SUCCESS;
	stream << rand;
	stream << std::uint64_t(0) << std::uint64_t(0);
    
	state_ = protocol::ClientOpcodes::CMSG_RECONNECT_PROOF;
	write(resp);
}

void LoginHandler::send_reconnect_proof() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(buffer_.size() != sizeof(protocol::ClientReconnectProof)) {
		LOG_DEBUG(logger_) << "Malformed reconnect proof from " << username_ << LOG_ASYNC;
		return;
	}

	auto packet = buffer_.data<protocol::ClientReconnectProof>();
	
	if(!auth_.reconnect_proof_check(packet)) {
		LOG_DEBUG(logger_) << "Failed to reconnect " << username_ << LOG_ASYNC;
		return;
	} else {
		LOG_DEBUG(logger_) << "Successfully reconnected " << username_ << LOG_ASYNC;
	}
	
	std::shared_ptr<Packet> resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(*resp);
	
	stream << protocol::ServerOpcodes::SMSG_RECONNECT_PROOF;
	stream << std::uint8_t(0) << std::uint16_t(0);

	state_ = protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST;
	write(resp);
}

} //ember