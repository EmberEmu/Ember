/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/PacketHeaders.h>
#include <game_protocol/Packets.h>
#include <botan/sha160.h>
#include <botan/bigint.h>
#include <spark/Buffer.h>
#include "../ClientStates.h"
#include <logger/Logging.h>

namespace ember {

class ClientHandler;
class ClientConnection;

class Authentication final {
	log::Logger* logger_;

	ClientHandler& handler_;
	ClientConnection& connection_;
	bool auth_done_;

	std::uint32_t auth_seed_;
	std::string account_name_;

	void send_auth_success();
	void send_auth_fail(protocol::ResultCode result);
	void handle_authentication(const protocol::ClientHeader& header, spark::Buffer& buffer);
	void prove_session(Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet);
	void fetch_session_key(const protocol::CMSG_AUTH_SESSION& packet);

public:
	Authentication(ClientHandler& handler, ClientConnection& connection, log::Logger* logger)
	               : handler_(handler), connection_(connection), logger_(logger),
	                 auth_seed_(0), auth_done_(false) { }

	void update(const protocol::ClientHeader& header, spark::Buffer& buffer);
	void send_auth_challenge();
};

} // ember