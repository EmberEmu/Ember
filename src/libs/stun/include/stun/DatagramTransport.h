/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/TransportBase.h>
#include <boost/asio.hpp>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::stun {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class DatagramTransport final : public Transport {
	ba::io_context ctx_;
	ba::ip::udp::socket socket_;
	ba::ip::udp::endpoint ep_;

	const std::chrono::milliseconds timeout_;
	const unsigned int retries_;

	void receive();
public:
	DatagramTransport(std::chrono::milliseconds timeout = 500ms, unsigned int retries = 7);
	~DatagramTransport() override;

	void connect(std::string_view host, std::uint16_t port) override;
	void send(std::vector<std::uint8_t> message) override;
	void close() override;
	std::chrono::milliseconds timeout() override;
	unsigned int retries() override;
	boost::asio::io_context* executor() override;
	std::string local_ip() override;
	std::uint16_t local_port() override;
};

} // stun, ember