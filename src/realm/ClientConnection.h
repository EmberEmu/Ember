/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ConnectionDefines.h"
#include "ConnectionStats.h"
#include "FilterTypes.h"
#include "Forwards.h"
#include "PacketCrypto.h"
#include "packet_log/PacketLogger.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <protocol/Concepts.h>
#include <shared/ClientIdent.h>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::realm {

class ClientHandler;

class ClientConnection final {
	static constexpr std::string_view allocator_tag { "realm_client_connection" };

public:
	static constexpr auto key_size = 40;

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
	InplaceAllocator allocator_;

	ClientHandler* handler_;
	ConnectionStats stats_;
	std::optional<PacketCrypto<key_size>> crypt_;
	protocol::SizeType msg_size_;
	log::Logger& logger_;
	bool write_in_progress_;
	unsigned int compression_level_;
	std::unique_ptr<PacketLogger> packet_logger_;
	EventDispatcher& dispatcher_;
	std::atomic_bool stopped_;
	ClientIdent ident_;

	// socket I/O
	void read();
	void write();

	// message reading & dispatching
	void process_buffered_data();
	void parse_header();
	void completion_check();
	void dispatch_message();

	void log_packet();
	bool write_packet_stream(const protocol::is_packet auto& packet);
	std::size_t minimum_transfer() const;
	void close_session();
	void set_handler(ClientHandler& handler);

public:
	ClientConnection(tcp_socket socket, const ClientIdent& ident, EventDispatcher& dispatcher, log::Logger& logger);
	~ClientConnection();

	// session management
	void start(ClientHandler& handler);
	void stop();
	bool stopped() const;

	void send(const protocol::is_packet auto& packet);
	void send(std::span<const std::uint8_t> packet);

	void set_key(std::span<const std::uint8_t, key_size> key);
	void compression_level(unsigned int level);
	void latency(std::uint32_t latency);

	const ConnectionStats& stats() const;
	std::string remote_address() const;

	void packet_log_start(std::unique_ptr<PacketLogger> logger);
	void packet_log_stop();
	bool packet_logging() const;
};

} // realm, ember

#include "ClientConnection.inl"