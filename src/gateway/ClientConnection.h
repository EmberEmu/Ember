/*
 * Copyright (c) 2016 - 2018 Ember
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
#include "packetlog/PacketLogger.h"
#include <game_protocol/PacketHeaders.h>
#include <spark/buffers/ChainedBuffer.h>
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <shared/memory/ASIOAllocator.h>
#include <botan/bigint.h>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember {

class SessionManager;

class ClientConnection final {
	static constexpr std::size_t INBOUND_SIZE = 1024;
	static constexpr std::size_t OUTBOUND_SIZE = 2048;

	// ClientHeader struct is not packed - do not do sizeof(protocol::ClientHeader)
	static constexpr std::size_t HEADER_WIRE_SIZE =
		sizeof(protocol::ClientHeader::opcode) + sizeof(protocol::ClientHeader::size);

	enum class ReadState { HEADER, BODY, DONE } read_state_;

	boost::asio::io_service& service_;
	boost::asio::ip::tcp::socket socket_;

	spark::ChainedBuffer<INBOUND_SIZE> inbound_buffer_;
	std::array<spark::ChainedBuffer<OUTBOUND_SIZE>, 2> outbound_buffers_;
	spark::ChainedBuffer<OUTBOUND_SIZE>* outbound_front_;
	spark::ChainedBuffer<OUTBOUND_SIZE>* outbound_back_;

	ClientHandler handler_;
	ConnectionStats stats_;
	PacketCrypto crypto_;
	protocol::SizeType msg_size_;
	SessionManager& sessions_;
	ASIOAllocator allocator_; // todo - should be shared & passed in
	log::Logger* logger_;
	bool authenticated_;
	bool write_in_progress_;
	unsigned int compression_level_;
	const std::string address_;
	std::unique_ptr<PacketLogger> packet_logger_;

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
	void dispatch_message(spark::Buffer& buffer);
	void process_buffered_data(spark::Buffer& buffer);
	void parse_header(spark::Buffer& buffer);
	void completion_check(const spark::Buffer& buffer);
	void stream_compress(const protocol::ServerPacket& packet);
	void swap_buffers();

public:
	ClientConnection(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	                 ClientUUID uuid, log::Logger* logger)
	                 : service_(socket.get_io_service()), sessions_(sessions),
	                   socket_(std::move(socket)), stats_{}, crypto_{}, msg_size_{0},
	                   logger_(logger), read_state_(ReadState::HEADER), stopped_(true),
	                   authenticated_(false), write_in_progress_(false),
	                   address_(boost::lexical_cast<std::string>(socket_.remote_endpoint())),
	                   handler_(*this, uuid, logger, socket.get_io_service()), compression_level_(0),
	                   outbound_front_(&outbound_buffers_[0]),
	                   outbound_back_(&outbound_buffers_[1]) { }

	void start();

	void set_authenticated(const Botan::BigInt& key);
	void compression_level(unsigned int level);
	void latency(std::size_t latency);

	const ConnectionStats& stats() const;
	std::string remote_address();

	// these should be made private, only for use by the handler
	void log_packets(bool enable);
	void send(const protocol::ServerPacket& packet);
	void close_session();
	void terminate();

	static void async_shutdown(const std::shared_ptr<ClientConnection>& client);
};

} // ember