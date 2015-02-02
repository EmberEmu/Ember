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
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <cstdint>

namespace ember {

class LoginHandler : public std::enable_shared_from_this<LoginHandler> {
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;
	boost::asio::deadline_timer timer_;
	ASIOAllocator& allocator_;
	log::Logger* logger_;
	PacketBuffer buffer_;
	Authenticator auth_;
	protocol::CMSG_OPCODE state_;
	bool new_session_ = true;

	void read();
	void write(std::shared_ptr<std::vector<char>> buffer);
	void handle_packet();
	void login_challenge_read();
	void process_login_challenge();
	void handle_client_proof();
	void close(const boost::system::error_code& error);

public:
	LoginHandler(boost::asio::ip::tcp::socket socket, ASIOAllocator& allocator,
	             Authenticator auth, log::Logger* logger)
	             : socket_(std::move(socket)), allocator_(allocator), logger_(logger),
	               timer_(socket_.get_io_service()), strand_(socket.get_io_service()),
	               auth_(std::move(auth)) { }

	void start();
};

} //ember