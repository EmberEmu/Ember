/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/TransportBase.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <queue>
#include <thread>
#include <cstddef>

namespace ember::stun {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class StreamTransport final : public Transport {
	enum class ReadState {
		READ_HEADER, READ_BODY, READ_DONE
	} state_ = ReadState::READ_HEADER;

	ba::io_context ctx_;
	ba::ip::tcp::socket socket_;
	ba::ip::tcp::resolver resolver_;
	std::jthread worker_;
	std::unique_ptr<boost::asio::io_context::work> work_;

	std::vector<std::uint8_t> buffer_;

	const std::chrono::milliseconds timeout_;
	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	std::size_t get_length();
	void read(std::size_t size, std::size_t offset);
	void receive();
	void do_write();
	void do_connect(ba::ip::tcp::resolver::results_type results, OnConnect&& cb);

public:
	StreamTransport(const std::string& bind, std::chrono::milliseconds timeout = 39500ms);
	~StreamTransport();

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