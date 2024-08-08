/*
 * Copyright (c) 2024 Ember
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
#include <thread>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::ports::upnp {

namespace ba = boost::asio;
using namespace std::chrono_literals;

class HTTPTransport final {
public:
	using Response = std::pair<HTTPHeader, std::span<const char>>;

private:
	static constexpr std::size_t INITIAL_BUFFER_SIZE = 65536u;
	static constexpr std::size_t MAX_BUFFER_SIZE = 1024u * 1024u;
	static constexpr auto READ_TIMEOUT = 60s;

	ba::ip::tcp::socket socket_;
	ba::ip::tcp::endpoint ep_;
	ba::ip::tcp::resolver resolver_;
	ba::steady_timer timeout_;
	std::vector<char> buffer_;

	bool buffer_resize(const std::size_t offset);
	void start_timer();
	ba::awaitable<std::size_t> read(std::size_t offset);
	bool http_headers_completion(std::size_t read);
	std::size_t http_body_completion(const HTTPHeader& header, std::size_t read);

public:
	HTTPTransport(ba::io_context& ctx, const std::string& bind);
	~HTTPTransport();

	ba::awaitable<Response> receive_http_response();
	ba::awaitable<void> connect(std::string_view host, std::uint16_t port);
	ba::awaitable<void> send(std::shared_ptr<std::vector<std::uint8_t>> message);
	ba::awaitable<void> send(std::vector<std::uint8_t> message);
	void close();
	bool is_open() const;
	ba::ip::tcp::endpoint local_endpoint() const;
};

} // upnp, ports, ember