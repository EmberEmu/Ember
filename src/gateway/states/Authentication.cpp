/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authentication.h"
#include "../ClientConnection.h"
#include <game_protocol/Opcodes.h>
#include <game_protocol/PacketHeaders.h>
#include <game_protocol/Packets.h>
#include <spark/Buffer.h>
#include <spark/temp/Account_generated.h>
#include <logger/Logging.h>
#include <botan/botan.h>
#include <botan/sha160.h>

#include "../temp.h"

#undef ERROR // temp

namespace em = ember::messaging;

namespace ember {

namespace {

void send_auth_challenge(ClientContext* ctx);
void send_auth_result(ClientContext* ctx, protocol::ResultCode result);
void handle_authentication(ClientContext* ctx);
void prove_session(ClientContext* ctx, Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet);
void fetch_session_key(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet);

void handle_authentication(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	// prevent repeated auth attempts
	if(ctx->auth_done) {
		return;
	} else {
		ctx->auth_done = true;
	}

	protocol::CMSG_AUTH_SESSION packet;
	packet.set_size(ctx->header->size - sizeof(protocol::ClientHeader::opcode));

	if(!ctx->handler->packet_deserialise(packet, *ctx->buffer)) {
		return;
	}

	// todo - check game build
	fetch_session_key(ctx, packet);
}

void fetch_session_key(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;
	LOG_DEBUG_GLOB << "Received session proof from " << packet.username << LOG_ASYNC;

	auto self = ctx->connection->shared_from_this();

	acct_serv->locate_session(packet.username, [self, ctx, packet](em::account::Status remote_res,
	                                                               Botan::BigInt key) {
		ctx->connection->socket().get_io_service().post([self, ctx, packet, remote_res, key]() {
			LOG_DEBUG_FILTER_GLOB(LF_NETWORK)
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
						LOG_ERROR_FILTER_GLOB(LF_NETWORK) << "Received "
							<< em::account::EnumNameStatus(remote_res)
							<< " from account server" << LOG_ASYNC;
						result = protocol::ResultCode::AUTH_SYSTEM_ERROR;
				}

				send_auth_result(ctx, result);
			} else {
				prove_session(ctx, key, packet);
			}
		});
	});
}

void send_auth_challenge(ClientContext* ctx) {
	auto packet = std::make_shared<protocol::SMSG_AUTH_CHALLENGE>();
	packet->seed = ctx->auth_seed = 600; // todo, obviously
	ctx->connection->send(packet);
}

void prove_session(ClientContext* ctx, Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet) {
	Botan::SecureVector<Botan::byte> k_bytes = Botan::BigInt::encode(key);
	std::uint32_t unknown = 0;

	Botan::SHA_160 hasher;
	hasher.update(packet.username);
	hasher.update((Botan::byte*)&unknown, sizeof(unknown));
	hasher.update((Botan::byte*)&packet.seed, sizeof(packet.seed));
	hasher.update((Botan::byte*)&ctx->auth_seed, sizeof(ctx->auth_seed));
	hasher.update(k_bytes);
	Botan::SecureVector<Botan::byte> calc_hash = hasher.final();

	if(calc_hash != packet.digest) {
		send_auth_result(ctx, protocol::ResultCode::AUTH_BAD_SERVER_PROOF);
		return;
	}

	ctx->connection->set_authenticated(key);
	ctx->account_name = packet.username;

	auto auth_success = [](ClientContext* ctx) {
		send_auth_result(ctx, protocol::ResultCode::AUTH_OK);
		// send_addon_data();
	};

	/*
	* Note: MaNGOS claims you need a full auth packet for the initial AUTH_WAIT_QUEUE
	* but that doesn't seem to be true - if this bugs out, check that out
	*/
	if(false) {
		ctx->state = ClientState::IN_QUEUE;

		queue_service_temp->enqueue(ctx->connection->shared_from_this(), [auth_success, ctx, packet]() {
			LOG_DEBUG_GLOB << packet.username << " removed from queue" << LOG_ASYNC;
			auth_success(ctx);
		});

		LOG_DEBUG_GLOB << packet.username << " added to queue" << LOG_ASYNC;
		return;
	}

	auth_success(ctx);
}

void send_auth_result(ClientContext* ctx, protocol::ResultCode result) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	// not convinced that this packet is correct, apart from for AUTH_OK
	auto response = std::make_shared<protocol::SMSG_AUTH_RESPONSE>();
	response->result = result;
	ctx->connection->send(response);
}

} // unnamed

namespace authentication {

void enter(ClientContext* ctx) {
	send_auth_challenge(ctx);
}

void update(ClientContext* ctx) {
	switch(ctx->header->opcode) {
		case protocol::ClientOpcodes::CMSG_AUTH_SESSION:
			handle_authentication(ctx);
			break;
		//default:
			//ctx->state = ClientState::UNEXPECTED_PACKET;
	}
}

void exit(ClientContext* ctx) {

}

}} // authentication, ember