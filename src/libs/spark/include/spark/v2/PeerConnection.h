/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <array>
#include <functional>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember::spark::v2 {

class Message;

class PeerConnection final {
public:
	using ReceiveHandler = std::function<void(std::span<const std::uint8_t>)>;

private:
	static constexpr auto MAX_MESSAGE_SIZE = 1024u ^ 2;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	std::array<std::uint8_t, MAX_MESSAGE_SIZE> buffer_{};
	std::queue<std::unique_ptr<Message>> queue_;
	std::size_t offset_{};

	boost::asio::awaitable<void> process_queue();
	boost::asio::awaitable<void> begin_receive(ReceiveHandler handler);
	boost::asio::awaitable<std::pair<std::size_t, std::uint32_t>> do_receive(std::size_t offset);
	boost::asio::awaitable<std::size_t> read_until(std::size_t offset, std::size_t read_size);

public:
	PeerConnection(boost::asio::ip::tcp::socket socket);
	PeerConnection(PeerConnection&&) = default;

	std::string address();
	void send(std::unique_ptr<Message> buffer);
	boost::asio::awaitable<void> send(Message& msg);
	boost::asio::awaitable<std::span<std::uint8_t>> receive_msg();
	void start(ReceiveHandler handler);
	void close();
};

} // v2, spark, ember