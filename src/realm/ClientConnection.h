/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "BindToken.h"
#include "ClientIdent.h"
#include "ConnectionDefines.h"
#include "ConnectionStats.h"
#include "FilterTypes.h"
#include "Forwards.h"
#include "PacketCrypto.h"
#include "packet_log/PacketLogger.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <protocol/Concepts.h>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember::realm {

class AllocationProvider;
class ClientHandler;

class CorkToken {
	unsigned int depth;

	CorkToken(unsigned int depth)
		: depth(depth) {}

	friend class ClientConnection;
};

enum class WriteResult {
	success,
	would_block,
	failed
};

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

	tcp::Socket socket_;
	boost::asio::ip::tcp::endpoint endpoint_;

	StaticBuffer inbound_buffer_;
	std::pair<DynamicTLSBuffer, DynamicTLSBuffer> outbound_buffers_;
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
	unsigned int cork_depth_;
	std::unique_ptr<PacketLogger> packet_logger_;
	EventDispatcher& dispatcher_;
	std::atomic_bool stopped_;
	ClientIdent ident_;

	// socket I/O
	void read();
	void write();
	void flush();
	WriteResult fastpath_write();

	// message reading & dispatching
	void process_buffered_data();
	void parse_header();
	void completion_check();
	void dispatch_message();

	void log_packet();

	template<protocol::is_packet PacketType>
	bool write_packet_stream(const PacketType& packet);

	std::size_t minimum_transfer() const;
	void close_session();
	void set_handler(ClientHandler& handler);
	void set_ident(const ClientIdent& ident);
	void set_non_blocking();

public:
	ClientConnection(tcp::Socket socket, boost::asio::ip::tcp::endpoint endpoint,
	                 std::pair<DynamicTLSBuffer, DynamicTLSBuffer> buffers,
	                 EventDispatcher& dispatcher, log::Logger& logger);
	~ClientConnection();

	ClientConnection(ClientConnection&& other) noexcept;
	ClientConnection& operator=(ClientConnection&& other) noexcept = delete;
	ClientConnection(ClientConnection& other) = delete;
	ClientConnection& operator=(ClientConnection& other) = delete;

	// session management
	BindToken<ClientConnection> bind(ClientHandler& handler, const ClientIdent& ident);
	void start(BindToken<ClientConnection> token);
	void stop();
	bool stopped() const;

	void send(const protocol::is_packet auto& packet);
	void send(std::span<const std::uint8_t> packet);

	void set_key(std::span<const std::uint8_t, key_size> key);
	void compression_level(unsigned int level);
	void latency(std::uint32_t latency);

	const ConnectionStats& stats() const;
	std::string address() const;

	void packet_log_start(std::unique_ptr<PacketLogger> logger);
	void packet_log_stop();
	bool packet_logging() const;

	void rebind(ClientHandler& handler) noexcept;

	CorkToken cork();
	void uncork(CorkToken token);
};

} // realm, ember

#include "ClientConnection.inl"