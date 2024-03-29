/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Dispatcher.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <array>
#include <queue>
#include <memory>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember::spark::v2 {

class PeerConnection final {
	static constexpr auto MAX_MESSAGE_SIZE = 1024u ^ 2;

	Dispatcher& dispatcher_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	std::array<std::uint8_t, MAX_MESSAGE_SIZE> buffer_;
	std::queue<std::unique_ptr<std::vector<std::uint8_t>>> queue_;
	bool sending_ = false;

	boost::asio::awaitable<void> process_queue();
	boost::asio::awaitable<std::size_t> read_until(std::size_t offset, std::size_t read_size);
	boost::asio::awaitable<void> receive();
	boost::asio::awaitable<std::size_t> do_receive(std::size_t offset);

public:
	PeerConnection(Dispatcher& dispatcher, boost::asio::ip::tcp::socket socket);
	PeerConnection(PeerConnection&&) = default;

	void send(std::unique_ptr<std::vector<std::uint8_t>> buffer);
	void close();
};

} // spark, ember