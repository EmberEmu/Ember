/*
 * Copyright (c) 2023 - 2024 Ember
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
#include <queue>
#include <thread>

namespace ember::stun {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class DatagramTransport final : public Transport {
	mutable ba::io_context ctx_;
	ba::ip::udp::socket socket_;
	ba::ip::udp::endpoint ep_;
	ba::ip::udp::endpoint remote_ep_;
	std::jthread worker_;

	const std::chrono::milliseconds timeout_;
	const unsigned int retries_;

	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	std::vector<std::uint8_t> buffer_;
	ba::ip::udp::resolver resolver_;

	void receive();
	void do_write();

public:
	DatagramTransport(const std::string& bind, std::chrono::milliseconds timeout = 500ms,
	                  unsigned int retries = 7);
	~DatagramTransport() override;

	void connect(std::string_view host, std::uint16_t port, OnConnect&& cb) override;
	void send(std::shared_ptr<std::vector<std::uint8_t>> message) override;
	void send(std::vector<std::uint8_t> message) override;
	void close() override;
	std::chrono::milliseconds timeout() const override;
	unsigned int retries() const override;
	std::string local_ip() const override;
	std::uint16_t local_port() const override;
};

} // stun, ember