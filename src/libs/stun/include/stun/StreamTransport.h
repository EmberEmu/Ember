/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Transport.h>
#include <boost/asio.hpp>
#include <span>

namespace ember::stun {

namespace ba = boost::asio;

class StreamTransport final : public Transport {
	ba::io_context ctx_;
	ba::ip::udp::socket socket_;

	const std::string host_;
	const std::uint16_t port_;

public:
	StreamTransport(const std::string& host, std::uint16_t port);

	void connect() override;
	void send(std::span<std::uint8_t> message) override;
	void receive();
	void close();
};

} // stun, ember