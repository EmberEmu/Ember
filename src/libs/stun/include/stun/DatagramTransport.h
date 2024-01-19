/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Transport.h>
#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <cstdint>

namespace ember::stun {

namespace ba = boost::asio;

class DatagramTransport final : public Transport {
	using ReceiveCallback = std::function<void(std::vector<std::uint8_t>)>;
	using OnConnectionError = std::function<void(const boost::system::error_code&)>;

	ba::io_context& ctx_;
	ba::ip::udp::socket socket_;
	ba::ip::udp::endpoint ep_;

	const std::string host_;
	const std::uint16_t port_;
	ReceiveCallback rcb_;
	OnConnectionError ecb_;

	void receive();
public:
	DatagramTransport(ba::io_context& ctx, const std::string& host, std::uint16_t port,
		ReceiveCallback rcb, OnConnectionError ecb);
	~DatagramTransport() override;

	void connect() override;
	void send(std::vector<std::uint8_t> message);
	void close();
};

} // stun, ember