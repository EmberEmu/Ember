/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <functional>
#include <string_view>
#include <memory>
#include <queue>
#include <thread>
#include <cstdint>
#include <span>

namespace ember::ports {

using namespace std::chrono_literals;

class DatagramTransport final {
	static const std::size_t INITIAL_RECV_BUFFER_SIZE = 2048;

	using OnReceive = std::function<void(std::span<std::uint8_t>, const boost::asio::ip::udp::endpoint&)>;
	using OnConnectionError = std::function<void(const boost::system::error_code&)>;
	using OnResolve = std::function<void(const boost::system::error_code&,
	                                     const boost::asio::ip::udp::endpoint& ep)>;

	OnReceive rcb_;
	OnConnectionError ecb_;
	OnResolve ocb_;

	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint ep_;
	boost::asio::ip::udp::endpoint remote_ep_;

	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	std::vector<std::uint8_t> buffer_;
	boost::asio::ip::udp::resolver resolver_;

	void receive();
	void do_write();

public:
	DatagramTransport(const std::string_view bind, std::uint16_t port, boost::asio::io_context& ctx_);
	~DatagramTransport();

	void set_callbacks(OnReceive rcb, OnConnectionError ecb);
	void resolve(const std::string_view host, std::uint16_t port, OnResolve&& cb);
	void send(std::shared_ptr<std::vector<std::uint8_t>> message);
	void send(std::vector<std::uint8_t> message);
	void join_group(const std::string_view address);
	void close();
};

} // ports, ember