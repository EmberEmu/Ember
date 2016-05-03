/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include "SessionManager.h"
#include "FilterTypes.h"
#include <game_protocol/Handler.h>
#include <logger/Logging.h>
#include <spark/buffers/ChainedBuffer.h>
#include <shared/PacketStream.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace ember {

class ClientConnection final : public std::enable_shared_from_this<ClientConnection> {
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_; // todo, remove

	ClientStates state_;
	protocol::Handler handler_;
	spark::ChainedBuffer<1024> inbound_buffer_;
	spark::ChainedBuffer<1024> outbound_buffer_;
	SessionManager& sessions_;
	ASIOAllocator allocator_; // temp - should be passed in
	log::Logger* logger_;
	bool stopped_;

	void read();
	void stop();

	void send_auth_challenge();

public:
	ClientConnection(SessionManager& sessions, boost::asio::io_service& service, log::Logger* logger)
	                 : state_(ClientStates::INITIAL_CONNECTION), sessions_(sessions), socket_(service),
	                   strand_(service), logger_(logger), stopped_(false) { }

	void start();
	boost::asio::ip::tcp::socket& socket();
	void close_session();
	bool handle_packet(spark::Buffer& buffer);

	template<std::size_t BlockSize>
	void write_chain(std::shared_ptr<spark::ChainedBuffer<BlockSize>> chain);
	
	std::string remote_address();
	std::uint16_t remote_port();

	friend class SessionManager;
};

} // ember