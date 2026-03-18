/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/HTTPTypes.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <expected>
#include <memory>
#include <queue>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::ports::upnp {

using namespace std::chrono_literals;

class HTTPTransport final {
public:
	using Response = std::pair<HTTPHeader, std::span<const char>>;

private:
	static constexpr std::size_t INITIAL_BUFFER_SIZE = 65536u;
	static constexpr std::size_t MAX_BUFFER_SIZE = 1024u * 1024u;
	static constexpr auto READ_TIMEOUT = 60s;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::endpoint ep_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::asio::steady_timer timeout_;
	std::vector<char> buffer_;

	bool buffer_resize(const std::size_t offset);
	void start_timer();
	void stop_timer();
	boost::asio::awaitable<std::size_t> read(std::size_t offset);
	bool http_headers_completion(std::size_t read);
	std::size_t http_body_completion(const HTTPHeader& header, std::size_t read);

public:
	HTTPTransport(boost::asio::io_context& ctx, std::string_view bind);
	~HTTPTransport();

	boost::asio::awaitable<Response> receive_http_response();
	boost::asio::awaitable<void> connect(const std::string_view host, std::uint16_t port);
	boost::asio::awaitable<void> send(std::shared_ptr<std::vector<std::uint8_t>> message);
	boost::asio::awaitable<void> send(std::vector<std::uint8_t> message);
	void close();
	bool is_open() const;
	boost::asio::ip::tcp::endpoint local_endpoint() const;
};

} // upnp, ports, ember