/*
* Copyright (c) 2024 - 2026 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <ports/upnp/MulticastSocket.h>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <utility>

namespace asio = boost::asio;

namespace ember::ports {

MulticastSocket::MulticastSocket(asio::io_context& context,
								 std::string_view listen_iface,
								 std::string_view mcast_group,
								 const std::uint16_t port)
	: context_(context)
	, socket_(context)
	, buffer_{}
	, ep_(asio::ip::make_address(mcast_group), port) {
	const auto mcast_iface = asio::ip::make_address(listen_iface);
	const auto group_ip = asio::ip::make_address(mcast_group);

	asio::ip::udp::endpoint ep(mcast_iface, 0);
	socket_.open(ep.protocol());
	socket_.set_option(asio::ip::udp::socket::reuse_address(true));
	asio::ip::multicast::join_group join_opt{};

	// Asio is doing something weird on Windows, this is a hack
	if(mcast_iface.is_v4()) {
		join_opt = asio::ip::multicast::join_group(group_ip.to_v4(), mcast_iface.to_v4());
	} else {
		join_opt = asio::ip::multicast::join_group(group_ip);
	}

	socket_.set_option(join_opt);
	socket_.bind(ep);
}

MulticastSocket::~MulticastSocket() {
	close();
}

auto MulticastSocket::receive() -> asio::awaitable<Result> {
	if(!socket_.is_open()) {
		co_return std::unexpected(asio::error::not_connected);
	}

	auto buffer = asio::buffer(buffer_);
	auto [ec, size] = co_await socket_.async_receive_from(buffer, remote_ep_, asio::as_tuple);

	if(ec) {
		co_return std::unexpected(ec);
	}

	co_return std::span { buffer_.data(), size };
}

asio::awaitable<bool> MulticastSocket::send(std::span<const std::uint8_t> buffer) {
	co_return co_await send(buffer, ep_);
}

asio::awaitable<bool> MulticastSocket::send(std::span<const std::uint8_t> buffer, asio::ip::udp::endpoint ep) {
	if(!socket_.is_open()) {
		co_return false;
	}

	const auto& [ec, _] = co_await socket_.async_send_to(buffer, ep, asio::as_tuple);

	if(ec) {
		socket_.close();
		co_return false;
	}

	co_return true;
}

void MulticastSocket::close() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(asio::ip::udp::socket::shutdown_both, ec);
	socket_.close(ec);
}

std::string MulticastSocket::local_address() const {
	return socket_.local_endpoint().address().to_string();
}

} // ports, ember