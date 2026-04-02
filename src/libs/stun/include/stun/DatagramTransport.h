/*
 * Copyright (c) 2023 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/TransportBase.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <queue>
#include <thread>

namespace ember::stun {

using namespace std::chrono_literals;

class DatagramTransport final : public Transport {
	static const std::size_t initial_recv_buffer_size = 2048;

	boost::asio::io_context& ctx_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint ep_;
	boost::asio::ip::udp::endpoint remote_ep_;

	const std::chrono::milliseconds timeout_;
	const unsigned int retries_;

	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	std::vector<std::uint8_t> buffer_;
	boost::asio::ip::udp::resolver resolver_;

	void receive();
	void do_write();

public:
	DatagramTransport(boost::asio::io_context& ctx,
	                  std::string_view bind,
	                  std::chrono::milliseconds timeout = 500ms,
	                  unsigned int retries = 7);

	~DatagramTransport();

	void connect(const std::string_view host, std::uint16_t port, OnConnect&& cb) override;
	void send(std::shared_ptr<std::vector<std::uint8_t>> message) override;
	void send(std::vector<std::uint8_t> message) override;
	void close() override;
	std::chrono::milliseconds timeout() const override;
	unsigned int retries() const override;
	std::string local_ip() const override;
	std::uint16_t local_port() const override;
};

} // stun, ember