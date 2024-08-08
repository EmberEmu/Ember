/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <ports/upnp/MulticastSocket.h>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <utility>

namespace ember::ports {

MulticastSocket::MulticastSocket(boost::asio::io_context& context,
								 const std::string& listen_iface,
								 const std::string& mcast_group,
								 const std::uint16_t port)
	: context_(context), socket_(context),
	buffer_{},
	ep_(boost::asio::ip::address::from_string(mcast_group), port) {
	const auto mcast_iface = boost::asio::ip::address::from_string(listen_iface);
	const auto group_ip = boost::asio::ip::address::from_string(mcast_group);

	boost::asio::ip::udp::endpoint ep(mcast_iface, 0);
	socket_.open(ep.protocol());
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	boost::asio::ip::multicast::join_group join_opt{};

	// ASIO is doing something weird on Windows, this is a hack
	if(mcast_iface.is_v4()) {
		join_opt = boost::asio::ip::multicast::join_group(group_ip.to_v4(), mcast_iface.to_v4());
	} else {
		join_opt = boost::asio::ip::multicast::join_group(group_ip);
	}

	socket_.set_option(join_opt);
	socket_.bind(ep);
}

MulticastSocket::~MulticastSocket() {
	close();
}

auto MulticastSocket::receive() -> ba::awaitable<ReceiveType> {
	if(!socket_.is_open()) {
		co_return std::unexpected(ba::error::not_connected);
	}

	auto buffer = boost::asio::buffer(buffer_.data(), buffer_.size());
	auto [ec, size] = co_await socket_.async_receive_from(buffer, remote_ep_, as_tuple(ba::deferred));

	if(ec) {
		co_return std::unexpected(ec);
	}

	co_return std::span{ buffer_.data(), size };
}

ba::awaitable<bool> MulticastSocket::send(std::vector<std::uint8_t> buffer, ba::ip::udp::endpoint ep) {
	auto ptr = std::make_shared<decltype(buffer)>(std::move(buffer));
	co_return co_await send(std::move(ptr), ep);
}

ba::awaitable<bool> MulticastSocket::send(std::vector<std::uint8_t> buffer) {
	auto ptr = std::make_shared<decltype(buffer)>(std::move(buffer));
	co_return co_await send(std::move(ptr));
}

ba::awaitable<bool> MulticastSocket::send(std::shared_ptr<std::vector<std::uint8_t>> buffer,
										  ba::ip::udp::endpoint ep) {
	if(!socket_.is_open()) {
		co_return false;
	}

	const auto ba_buf = boost::asio::buffer(*buffer);

	const auto& [ec, _] = co_await socket_.async_send_to(ba_buf, ep, as_tuple(ba::deferred));

	if(ec) {
		socket_.close();
		co_return false;
	}

	co_return true;
}

ba::awaitable<bool> MulticastSocket::send(std::shared_ptr<std::vector<std::uint8_t>> buffer) {
	co_return co_await send(std::move(buffer), ep_);
}

void MulticastSocket::close() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
	socket_.close(ec);
}

std::string MulticastSocket::local_address() const {
	return socket_.local_endpoint().address().to_string();
}

} // ports, ember