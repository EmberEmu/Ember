/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authentication.h"
#include "../ClientConnection.h"
#include <spark/temp/Account_generated.h>
#include <botan/secmem.h>

#include "../temp.h"

#undef ERROR // temp

namespace em = ember::messaging;

namespace ember {

void Authentication::update(const protocol::ClientHeader& header, spark::Buffer& buffer) {
	switch(header.opcode) {
		case protocol::ClientOpcodes::CMSG_AUTH_SESSION:
			handle_authentication(header, buffer);
			break;
		default:
			handler_.update_state(ClientState::UNEXPECTED_PACKET);
	}
}

void Authentication::handle_authentication(const protocol::ClientHeader& header, spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	// prevent repeated auth attempts
	if(auth_done_) {
		return;
	} else {
		auth_done_ = true;
	}

	protocol::CMSG_AUTH_SESSION packet;

	if(!handler_.packet_deserialise(packet, buffer)) {
		return;
	}

	// todo - check game build
	fetch_session_key(packet);
}

void Authentication::fetch_session_key(const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
	LOG_DEBUG(logger_) << "Received session proof from " << packet.username << LOG_ASYNC;

	auto self = connection_.shared_from_this();

	acct_serv->locate_session(packet.username,
		[this, self, packet](em::account::Status remote_res, Botan::BigInt key) {
			connection_.socket().get_io_service().post([this, self, packet, remote_res, key]() {
				LOG_DEBUG_FILTER(logger_, LF_NETWORK)
					<< "Account server returned " << em::account::EnumNameStatus(remote_res)
					<< " for " << packet.username << LOG_ASYNC;

				if(remote_res != em::account::Status::OK) {
					protocol::ResultCode result;

					switch(remote_res) {
						case em::account::Status::ALREADY_LOGGED_IN:
							result = protocol::ResultCode::AUTH_ALREADY_ONLINE;
							break;
						case em::account::Status::SESSION_NOT_FOUND:
							result = protocol::ResultCode::AUTH_UNKNOWN_ACCOUNT;
							break;
						default:
							LOG_ERROR_FILTER(logger_, LF_NETWORK) << "Received "
								<< em::account::EnumNameStatus(remote_res)
								<< " from account server" << LOG_ASYNC;
							result = protocol::ResultCode::AUTH_SYSTEM_ERROR;
					}

					send_auth_fail(result);
				} else {
					prove_session(key, packet);
				}
			});
	});
}

void Authentication::send_auth_challenge() {
	auto packet = std::make_shared<protocol::SMSG_AUTH_CHALLENGE>();
	packet->seed = auth_seed_ = 600; // todo, obviously

	connection_.send(protocol::ServerOpcodes::SMSG_AUTH_CHALLENGE, packet);
}

void Authentication::prove_session(Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet) {
	Botan::SecureVector<Botan::byte> k_bytes = Botan::BigInt::encode(key);
	std::uint32_t unknown = 0;

	Botan::SHA_160 hasher;
	hasher.update(packet.username);
	hasher.update((Botan::byte*)&unknown, sizeof(unknown));
	hasher.update((Botan::byte*)&packet.seed, sizeof(packet.seed));
	hasher.update((Botan::byte*)&auth_seed_, sizeof(auth_seed_));
	hasher.update(k_bytes);
	Botan::SecureVector<Botan::byte> calc_hash = hasher.final();

	if(calc_hash != packet.digest) {
		send_auth_fail(protocol::ResultCode::AUTH_BAD_SERVER_PROOF);
		return;
	}

	connection_.set_authenticated(key);
	account_name_ = packet.username;

	auto auth_success = [this]() {
		send_auth_success();
		// send_addon_data();
	};

	/* 
	 * Note: MaNGOS claims you need a full auth packet for the initial AUTH_WAIT_QUEUE
	 * but that doesn't seem to be true - if this bugs out, check that out
	 */
	if(false) {
		handler_.update_state(ClientState::IN_QUEUE);

		queue_service_temp->enqueue(connection_.shared_from_this(), [this, auth_success, packet]() {
			LOG_DEBUG(logger_) << packet.username << " removed from queue" << LOG_ASYNC;
			auth_success();
		});

		LOG_DEBUG(logger_) << packet.username << " added to queue" << LOG_ASYNC;
		return;
	}

	auth_success();
}

void Authentication::send_auth_success() {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
	handler_.update_state(ClientState::CHARACTER_LIST);

	auto response = std::make_shared<protocol::SMSG_AUTH_RESPONSE>();
	response->result = protocol::ResultCode::AUTH_OK;
	connection_.send(protocol::ServerOpcodes::SMSG_AUTH_RESPONSE, response);
}

void Authentication::send_auth_fail(protocol::ResultCode result) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
	handler_.update_state(ClientState::REQUEST_CLOSE);

	// not convinced that this packet is correct
	auto response = std::make_shared<protocol::SMSG_AUTH_RESPONSE>();
	response->result = result;
	connection_.send(protocol::ServerOpcodes::SMSG_AUTH_RESPONSE, response);
}

} // ember