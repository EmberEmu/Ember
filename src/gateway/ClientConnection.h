/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientHandler.h"
#include "ConnectionStats.h"
#include "PacketCrypto.h"
#include "FilterTypes.h"
#include <game_protocol/PacketHeaders.h> // todo, remove
#include <spark/buffers/ChainedBuffer.h>
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <shared/memory/ASIOAllocator.h>
#include <botan/bigint.h>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <cstdint>

namespace ember {

class SessionManager;

class ClientConnection final {
	enum class ReadState { HEADER, BODY, DONE } read_state_;

	boost::asio::io_service& service_;
	boost::asio::ip::tcp::socket socket_;

	spark::ChainedBuffer<1024> inbound_buffer_;
	spark::ChainedBuffer<4096> outbound_buffer_;

	ClientHandler handler_;
	ConnectionStats stats_;
	PacketCrypto crypto_;
	protocol::ClientHeader packet_header_;
	SessionManager& sessions_;
	ASIOAllocator allocator_; // todo - should be shared & passed in
	log::Logger* logger_;
	bool authenticated_;
	bool write_in_progress_;
	const std::string address_;

	std::condition_variable stop_condvar_;
	std::mutex stop_lock_;
	std::atomic_bool stopped_;

	// socket I/O
	void read();
	void write();

	// session management
	void stop();
	void close_session_sync();

	// packet reassembly & dispatching
	void process_buffered_data(spark::Buffer& buffer);
	void parse_header(spark::Buffer& buffer);
	void completion_check(spark::Buffer& buffer);

public:
	ClientConnection(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	                 ClientUUID uuid, log::Logger* logger)
	                 : service_(socket.get_io_service()), sessions_(sessions),
	                   socket_(std::move(socket)), stats_{}, crypto_{}, packet_header_{},
	                   logger_(logger), read_state_(ReadState::HEADER), stopped_(true),
	                   authenticated_(false), write_in_progress_(false),
	                   address_(boost::lexical_cast<std::string>(socket_.remote_endpoint())),
	                   handler_(*this, uuid, logger) {}

	~ClientConnection();

	void start();
	void close_session();

	void set_authenticated(const Botan::BigInt& key);
	void compression_level(unsigned int level);
	void latency(std::size_t latency);

	const ConnectionStats& stats() const;
	const ClientUUID& uuid() const;

	void send(const protocol::ServerPacket& packet);
	std::string remote_address();
};

} // ember