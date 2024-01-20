/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/TransportBase.h>
#include <boost/asio.hpp>
#include <string_view>

namespace ember::stun {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class StreamTransport final : public TransportBase {
	enum class ReadState {
		READ_HEADER, READ_BODY, READ_DONE
	} state_ = ReadState::READ_HEADER;

	ba::io_context ctx_;
	ba::ip::tcp::socket socket_;
	ba::ip::tcp::endpoint ep_;

	const std::string host_;
	const std::uint16_t port_;
	std::vector<std::uint8_t> buffer_;

	const std::chrono::milliseconds timeout_;

	std::size_t get_length();
	void read(std::size_t size, std::size_t offset);
	void receive();
public:
	StreamTransport(std::string_view host, std::uint16_t port,
		std::chrono::milliseconds timeout = 39500ms);
	~StreamTransport() override;

	void connect() override;
	void send(std::vector<std::uint8_t> message) override;
	void close() override;
	std::chrono::milliseconds timeout() override;
	unsigned int retries() override;
	boost::asio::io_context* executor() override;
};

} // stun, ember