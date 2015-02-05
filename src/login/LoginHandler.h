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
	bool new_session_ = true;

	void read();
	void write(std::shared_ptr<std::vector<char>> buffer);
	void handle_packet();
	void login_challenge_read();
	void process_login_challenge();
	void handle_login_proof();
	void close(const boost::system::error_code& error);
	void handle_login(Authenticator::ACCOUNT_STATUS status);
	void handle_reconnect_proof();
	void handle_reconnect(bool key_found);
	void send_server_challenge(PacketStream<Packet>& resp);

public:
	LoginHandler(boost::asio::ip::tcp::socket socket, ASIOAllocator& allocator,
	             Authenticator auth, ThreadPool& pool, log::Logger* logger)
	             : socket_(std::move(socket)), allocator_(allocator), logger_(logger),
	               timer_(socket_.get_io_service()), strand_(socket_.get_io_service()),
	               auth_(std::move(auth)), tpool_(pool), service_(socket_.get_io_service()) { }

	void start();
};

} //ember