/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ConnectionStats.h"
#include "ConnectionDefines.h"
#include "PacketCrypto.h"
#include "FilterTypes.h"
#include "packet_log/PacketLogger.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <spark/buffers/DynamicBuffer.h>
#include <shared/ClientIdent.h>
#include <shared/memory/AsioAllocator.h>
#include <botan/bigint.h>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember::realm {

class ClientHandler;

class ClientConnection final {
public:
	using OnDisconnect = std::function<void()>;

private:
	enum class ReadState {
		header,
		body,
		done
	} read_state_;

	tcp_socket socket_;
	boost::asio::ip::tcp::endpoint remote_ep_;

	StaticBuffer inbound_buffer_;
	std::array<DynamicTLSBuffer, 2> outbound_buffers_;
	DynamicTLSBuffer* outbound_front_;
	DynamicTLSBuffer* outbound_back_;

	ClientHandler* handler_;
	ConnectionStats stats_;
	std::optional<PacketCrypto<0>> crypt_;
	protocol::SizeType msg_size_;
	AsioAllocator<thread_safe> allocator_; // todo - should be shared & passed in
	log::Logger& logger_;
	bool write_in_progress_;
	unsigned int compression_level_;
	std::unique_ptr<PacketLogger> packet_logger_;
	OnDisconnect on_disconnect_;

	std::condition_variable stop_condvar_;
	std::mutex stop_lock_;
	std::atomic_bool stopped_;
	bool stopping_;

	// socket I/O
	void read();
	void write();

	// message reading & dispatching
	void process_buffered_data();
	void parse_header();
	void completion_check();
	void dispatch_message();

	bool write_packet_stream(const is_packet auto& packet);

	std::size_t minimum_transfer() const;
	void close_session();

public:
	ClientConnection(tcp_socket socket, log::Logger& logger)
		: socket_(std::move(socket))
		, remote_ep_(socket_.remote_endpoint())
		, stats_{}
		, msg_size_{0}
		, logger_(logger)
		, read_state_(ReadState::header)
		, stopped_(false)
		, write_in_progress_(false)
		, handler_(nullptr)
		, compression_level_(0)
		, outbound_front_(&outbound_buffers_.front())
		, outbound_back_(&outbound_buffers_.back())
		, stopping_(false) {}

	~ClientConnection();

	// session management
	void start();
	void stop();

	void set_key(std::span<const std::uint8_t> key);
	void compression_level(unsigned int level);
	void latency(std::uint32_t latency);

	const ConnectionStats& stats() const;
	std::string remote_address() const;
	void log_packets(bool enable);

	void send(const is_packet auto& packet);
	void set_handler(ClientHandler* handler);
	void set_on_disconnect(OnDisconnect on_disconnect);

	friend class ClientHandler;
};

} // realm, ember

#include "ClientConnection.inl"