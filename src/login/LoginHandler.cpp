/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include "Patcher.h"
#include <boost/range/adaptor/map.hpp>
#include <utility>

namespace ember {

bool LoginHandler::update_state(PacketBuffer& buffer) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(state_) {
		case State::INITIAL_CHALLENGE:
			process_challenge(buffer);
			break;
		case State::LOGIN_PROOF:
			check_login_proof(buffer);
			break;
		case State::RECONNECT_PROOF:
			send_reconnect_proof(buffer);
			break;
		case State::REQUEST_REALMS:
			send_realm_list(buffer);
			break;
		case State::CLOSED:
			return false;
		default:
			LOG_DEBUG(logger_) << "Received packet out of sync" << LOG_ASYNC;
			return false;
	}

	return true;
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	state_ = State::CLOSED;
	return false;
}

bool LoginHandler::update_state(std::shared_ptr<Action> action) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(state_) {
		case State::FETCHING_USER:
			send_login_challenge(static_cast<FetchUserAction*>(action.get()));
			break;
		case State::FETCHING_SESSION:
			send_reconnect_challenge(static_cast<FetchSessionKeyAction*>(action.get()));
			break;
		case State::WRITING_SESSION:
			send_login_success(static_cast<StoreSessionAction*>(action.get()));
			break;
		case State::CLOSED:
			return false;
		default:
			LOG_WARN(logger_) << "Received action out of sync" << LOG_ASYNC;
			return false;
	}

	return true;
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	state_ = State::CLOSED;
	return false;
}

void LoginHandler::process_challenge(const PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!check_opcode(buffer, protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE)
		&& !check_opcode(buffer, protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE)) {
		throw std::runtime_error("Expected CMSG_*_CHALLENGE");
	}

	auto packet = buffer.data<const protocol::ClientLoginChallenge>();

	// The username in the packet isn't null-terminated, so don't try using it directly
	username_ = std::string(packet->username, packet->username_len);
	GameVersion version{packet->major, packet->minor, packet->patch, packet->build};

	LOG_DEBUG(logger_) << "Challenge: " << username_ << ", " << version << ", "
	                   << source_ << LOG_ASYNC;

	Patcher::PatchLevel patch_level = patcher_.check_version(version);

	switch(patch_level) {
		case Patcher::PatchLevel::OK:
			accept_client(packet->header.opcode);
			break;
		case Patcher::PatchLevel::TOO_NEW:
		case Patcher::PatchLevel::TOO_OLD:
			reject_client(version);
			break;
		case Patcher::PatchLevel::PATCH_AVAILABLE:
			patch_client(version);
			break;
	}
}

void LoginHandler::patch_client(const GameVersion& version) {
	// don't know yet
	//stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_VERSION_UPDATE;
	//write(packet);
}

void LoginHandler::accept_client(protocol::ClientOpcodes opcode) {
	std::shared_ptr<Action> action;

	switch(opcode) {
		case protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE:
			action = std::make_shared<FetchUserAction>(username_, user_src_);
			state_ = State::FETCHING_USER;
			break;
		case protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE:
			action = std::make_shared<FetchSessionKeyAction>(username_, user_src_);
			state_ = State::FETCHING_SESSION;
			break;
		default:
			BOOST_ASSERT(false, "Impossible accept_client condition");
	}

	execute_action(action);
}

void LoginHandler::reject_client(const GameVersion& version) {
	LOG_DEBUG(logger_) << "Rejecting client version " << version << LOG_ASYNC;
	state_ = State::CLOSED;

	auto packet = std::allocate_shared<Packet>(boost::fast_pool_allocator<Packet>());
	PacketStream<Packet> stream(packet.get());

	stream << protocol::ServerOpcodes::SMSG_LOGIN_CHALLENGE << std::uint8_t(0)
	       << protocol::ResultCodes::FAIL_VERSION_INVALID;

	send(packet);
}

void LoginHandler::build_login_challenge(PacketStream<Packet>& stream) {	
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto values = login_auth_->challenge_reply();

	// Server's public ephemeral key
	auto B = Botan::BigInt::encode_1363(values.B, protocol::PUB_KEY_LENGTH);
	std::reverse(B.begin(), B.end()); // Botan's buffers are big-endian, client is little-endian

	// Safe prime
	auto N = Botan::BigInt::encode_1363(values.gen.prime(), protocol::PRIME_LENGTH);
	std::reverse(N.begin(), N.end());
	
	/// Salt
	auto salt = Botan::BigInt::encode(values.salt);
	std::reverse(salt.begin(), salt.end());
	
	// Do the stream writing after encoding the values so it's not in a bad state if there's an exception
	stream << protocol::ResultCodes::SUCCESS;
	stream << B;
	stream << std::uint8_t(values.gen.generator().bytes());
	stream << std::uint8_t(values.gen.generator().to_u32bit());
	stream << std::uint8_t(protocol::PRIME_LENGTH);
	stream << N << salt;
	stream << (Botan::AutoSeeded_RNG()).random_vec(16); // Random bytes, for some reason
	stream << std::uint8_t(0); // unknown
}

void LoginHandler::send_login_challenge(FetchUserAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	state_ = State::CLOSED;
	
	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(resp.get());
	stream << protocol::ServerOpcodes::SMSG_LOGIN_CHALLENGE << std::uint8_t(0);

	try {
		if((user_ = action->get_result())) {
			login_auth_ = std::make_unique<LoginAuthenticator>(*user_);
			build_login_challenge(stream);
			state_ = State::LOGIN_PROOF;
		} else {
			// leaks information on whether the account exists (could send challenge anyway?)
			LOG_DEBUG(logger_) << "Account not found: " << username_ << LOG_ASYNC;
			stream << protocol::ResultCodes::FAIL_UNKNOWN_ACCOUNT;
		}
	} catch(dal::exception& e) {
		LOG_ERROR(logger_) << "DAL failure for " << username_ << " " << e.what() << LOG_ASYNC;
		stream << protocol::ResultCodes::FAIL_DB_BUSY;
	} catch(Botan::Exception& e) {
		LOG_ERROR(logger_) << "Encoding failure for " << username_ << " " << e.what() << LOG_ASYNC;
		stream << protocol::ResultCodes::FAIL_DB_BUSY;
	}
	
	send(resp);
}

void LoginHandler::send_reconnect_challenge(FetchSessionKeyAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	state_ = State::CLOSED;

	auto rand = Botan::AutoSeeded_RNG().random_vec(16);
	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(resp.get());

	stream << protocol::ServerOpcodes::SMSG_RECONNECT_CHALLENGE;
	
	try {
		boost::optional<std::string> key = action->get_result();
		
		if(key) {
			state_ = State::RECONNECT_PROOF;
			stream << protocol::ResultCodes::SUCCESS;
			reconn_auth_ = std::make_unique<ReconnectAuthenticator>(username_, *key, rand);
		} else {
			stream << protocol::ResultCodes::FAIL_NOACCESS;
		}
	} catch(dal::exception& e) {
		LOG_ERROR(logger_) << "Retrieving key for " << username_ << ": " << e.what() << LOG_ASYNC;
		stream << protocol::ResultCodes::FAIL_DB_BUSY;
	}

	stream << rand;
	stream << std::uint64_t(0) << std::uint64_t(0);

	send(resp);
}

void LoginHandler::check_login_proof(PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!check_opcode(buffer, protocol::ClientOpcodes::CMSG_LOGIN_PROOF)) {
		throw std::runtime_error("Expected CMSG_LOGIN_PROOF");
	}

	auto proof = login_auth_->proof_check(buffer.data<protocol::ClientLoginProof>());
	auto result = protocol::ResultCodes::FAIL_INCORRECT_PASSWORD;

	if(proof.match) {
		if(user_->banned()) {
			result = protocol::ResultCodes::FAIL_BANNED;
		} else if(user_->suspended()) {
			result = protocol::ResultCodes::FAIL_SUSPENDED;
		/*} else if(time) {
			res = protocol::RESULT::FAIL_NO_TIME;
		} else if(parental_controls) {
			res = protocol::RESULT::FAIL_PARENTAL_CONTROLS;*/
		} else {
			result = protocol::ResultCodes::SUCCESS;
		}
	}

	if(result == protocol::ResultCodes::SUCCESS) {
		state_ = State::WRITING_SESSION;
		server_proof_ = proof.server_proof;
		auto action = std::make_shared<StoreSessionAction>(*user_, source_, login_auth_->session_key(), user_src_);
		execute_action(action);
	} else {
		state_ = State::CLOSED;
		send_login_failure(result);
	}
}

void LoginHandler::send_login_failure(protocol::ResultCodes result) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(resp.get());

	stream << protocol::ServerOpcodes::SMSG_LOGIN_PROOF;
	stream << result;

	send(resp);
}

void LoginHandler::send_login_success(StoreSessionAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	Botan::SecureVector<Botan::byte> m2 = Botan::BigInt::encode_1363(server_proof_, 20);
	std::reverse(m2.begin(), m2.end());

	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(resp.get());

	stream << protocol::ServerOpcodes::SMSG_LOGIN_PROOF;
	stream << protocol::ResultCodes::SUCCESS;
	stream << m2 << std::uint32_t(0); //proof << account flags

	state_ = State::REQUEST_REALMS;
	send(resp);
}

void LoginHandler::send_reconnect_proof(const PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!check_opcode(buffer, protocol::ClientOpcodes::CMSG_RECONNECT_PROOF)) {
		throw std::runtime_error("Expected CMSG_RECONNECT_PROOF");
	}

	if(reconn_auth_->proof_check(buffer.data<const protocol::ClientReconnectProof>())) {
		LOG_DEBUG(logger_) << "Successfully reconnected " << username_ << LOG_ASYNC;
	} else {
		LOG_DEBUG(logger_) << "Failed to reconnect " << username_ << LOG_ASYNC;
		return;
	}
	
	auto resp = std::make_shared<Packet>();
	PacketStream<Packet> stream(resp.get());
	
	stream << protocol::ServerOpcodes::SMSG_RECONNECT_PROOF;
	stream << protocol::ResultCodes::SUCCESS;

	state_ = State::REQUEST_REALMS;
	send(resp);
}

void LoginHandler::send_realm_list(const PacketBuffer& buffer) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!check_opcode(buffer, protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST)) {
		throw std::runtime_error("Expected CMSG_REQUEST_REALM_LIST");
	}

	auto header = std::make_shared<Packet>();
	auto body = std::make_shared<Packet>();
	PacketStream<Packet> stream(body.get());

	std::shared_ptr<const RealmMap> realms = realm_list_.realms();

	stream << std::uint32_t(0); // unknown 
	stream << std::uint8_t(realms->size());

	for(auto& realm : *realms | boost::adaptors::map_values) {
		stream << realm.icon;
		stream << realm.flags;
		stream << realm.name << std::uint8_t(0);
		stream << realm.ip << std::uint8_t(0);
		stream << realm.population;
		stream << std::uint8_t(0); // num chars
		stream << realm.timezone;
		stream << std::uint8_t(0); // unknown
	}

	stream << uint16_t(5); // unknown

	stream.swap(header.get());

	stream << protocol::ServerOpcodes::SMSG_REQUEST_REALM_LIST;
	stream << std::uint16_t(body->size());
	
	send(header);
	send(body);
}

bool LoginHandler::check_opcode(const PacketBuffer& buffer, protocol::ClientOpcodes opcode) {
	return *buffer.data<const protocol::ClientOpcodes>() == opcode;
}

// todo, remove in VS2015
LoginHandler& LoginHandler::operator=(LoginHandler&& rhs) {
	login_auth_ = std::move(rhs.login_auth_);
	reconn_auth_ = std::move(rhs.reconn_auth_);
	source_ = std::move(rhs.source_);
	return *this;
}

// todo, remove in VS2015
LoginHandler::LoginHandler(LoginHandler&& rhs)
                           : login_auth_(std::move(rhs.login_auth_)),
                             reconn_auth_(std::move(rhs.reconn_auth_)),
							 source_(rhs.source_), patcher_(rhs.patcher_),
                             user_src_(rhs.user_src_),
                             logger_(rhs.logger_), realm_list_(rhs.realm_list_) {}

} // ember