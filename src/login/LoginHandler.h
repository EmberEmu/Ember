/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "PacketBuffer.h"
#include "Protocol.h"
#include "Authenticator.h"
#include <logger/Logger.h>
#include <shared/threading/ThreadPool.h>
#include <shared/misc/PacketStream.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <cstdint>

namespace ember {

class LoginHandler : public std::enable_shared_from_this<LoginHandler> {
	enum STATE { INITIAL, LOGIN_CHALLENGE, LOGIN_PROOF,
	             RECONN_CHALLENGE, RECONN_PROOF, REQUEST_REALMS };

	boost::asio::ip::tcp::socket socket_;
	boost::asio::io_service& service_;
	boost::asio::strand strand_;
	boost::asio::deadline_timer timer_;
	ASIOAllocator& allocator_;
	log::Logger* logger_;
	PacketBuffer buffer_;
	Authenticator auth_;
	ThreadPool& tpool_;
	std::string username_;
	protocol::CMSG_OPCODE state_;
	bool initial_ = true;

	void read();
	void write(std::shared_ptr<std::vector<char>> buffer);
	void close(const boost::system::error_code& error);

	void handle_packet();
	void read_challenge();
	void process_challenge();
	void build_login_challenge(PacketStream<Packet>& resp);
	void send_login_challenge(Authenticator::ACCOUNT_STATUS status);
	void send_login_proof();
	void send_reconnect_challenge(bool key_found);
	void send_reconnect_proof();

public:
	LoginHandler(boost::asio::ip::tcp::socket socket, ASIOAllocator& allocator,
	             Authenticator auth, ThreadPool& pool, log::Logger* logger)
	             : socket_(std::move(socket)), allocator_(allocator), logger_(logger),
	               timer_(socket_.get_io_service()), strand_(socket_.get_io_service()),
	               auth_(std::move(auth)), tpool_(pool), service_(socket_.get_io_service()) { }

	void start();
};

} //ember