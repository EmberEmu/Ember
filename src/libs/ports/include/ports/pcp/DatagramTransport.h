/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <string>
#include <memory>
#include <queue>
#include <thread>
#include <cstdint>

namespace ember::ports {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class DatagramTransport final {
	using OnReceive = std::function<void(std::span<std::uint8_t>, const ba::ip::udp::endpoint&)>;
	using OnConnectionError = std::function<void(const boost::system::error_code&)>;
	using OnResolve = std::function<void(const boost::system::error_code&,
	                                     const ba::ip::udp::endpoint& ep)>;

	OnReceive rcb_;
	OnConnectionError ecb_;
	OnResolve ocb_;

	ba::io_context& ctx_;
	ba::io_context::strand strand_;
	ba::ip::udp::socket socket_;
	ba::ip::udp::endpoint ep_;
	ba::ip::udp::endpoint remote_ep_;

	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	std::vector<std::uint8_t> buffer_;
	ba::ip::udp::resolver resolver_;

	void receive();
	void do_write();

public:
	DatagramTransport(const std::string& bind, ba::io_context& ctx_);
	~DatagramTransport();

	void set_callbacks(OnReceive rcb, OnConnectionError ecb);
	void resolve(std::string_view host, std::uint16_t port, OnResolve&& cb);
	void send(std::shared_ptr<std::vector<std::uint8_t>> message);
	void send(std::vector<std::uint8_t> message);
	void join_group(const std::string& address);
	void close();
};

} // ports, ember