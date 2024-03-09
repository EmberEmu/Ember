/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/HTTPHeaderParser.h>
#include <boost/asio.hpp>
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
	static constexpr std::size_t INITIAL_BUFFER_SIZE = 65536u;
	static constexpr std::size_t MAX_BUFFER_SIZE = 1024u * 1024u;
	static constexpr auto READ_TIMEOUT = 60s;

	using OnReceive = std::function<void(const HTTPHeader& header, std::span<char>)>;
	using OnConnectionError = std::function<void(const boost::system::error_code&)>;
	using OnConnect = std::function<void(const boost::system::error_code&)>;

	ba::ip::tcp::socket socket_;
	ba::ip::tcp::endpoint ep_;
	ba::ip::tcp::resolver resolver_;
	ba::steady_timer timeout_;

	std::vector<char> buffer_;

	OnReceive rcv_cb_;
	OnConnectionError err_cb_;

	bool buffer_resize(const std::size_t offset);
	std::queue<std::shared_ptr<std::vector<std::uint8_t>>> queue_;
	void start_timer();
	void read(std::size_t offset);
	void receive(std::size_t read);
	void do_write();
	void do_connect(ba::ip::tcp::resolver::results_type results, OnConnect&& cb);

public:
	HTTPTransport(ba::io_context& ctx, const std::string& bind);
	~HTTPTransport();

	void connect(std::string_view host, std::uint16_t port, OnConnect&& cb);
	void send(std::shared_ptr<std::vector<std::uint8_t>> message);
	void send(std::vector<std::uint8_t> message);
	void close();
	bool is_open() const;
	ba::ip::tcp::endpoint local_endpoint() const;

	void set_callbacks(OnReceive receive, OnConnectionError error);
};

} // upnp, ports, ember