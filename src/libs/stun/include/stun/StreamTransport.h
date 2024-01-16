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
	using ReceiveCallback = std::function<void(std::vector<std::uint8_t>)>;

	enum class ReadState {
		READ_HEADER, READ_BODY, READ_DONE
	} state_ = ReadState::READ_HEADER;

	ba::io_context& ctx_;
	ba::ip::tcp::socket socket_;
	ba::ip::tcp::endpoint ep_;

	const std::string host_;
	const std::uint16_t port_;
	ReceiveCallback rcb_;
	std::vector<std::uint8_t> buffer_;

	std::size_t get_length();
	void read(std::size_t size, std::size_t offset);
	void receive();
public:
	StreamTransport(ba::io_context& ctx, const std::string& host, std::uint16_t port, ReceiveCallback rcb);
	~StreamTransport() override;

	void connect() override;
	void send(std::vector<std::uint8_t> message) override;
	void close();
};

} // stun, ember