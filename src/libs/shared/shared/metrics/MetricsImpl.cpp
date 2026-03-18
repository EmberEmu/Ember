/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/metrics/MetricsImpl.h>
#include <boost/asio/connect.hpp>
#include <format>
#include <functional>
#include <memory>
#include <sstream>
#include <utility>
#include <cstddef>

namespace ember {

MetricsImpl::MetricsImpl(boost::asio::io_context& service, const std::string& host,
                         std::uint16_t port)
	: socket_(service) {
	boost::asio::ip::udp::resolver resolver(service);
	const auto results = resolver.resolve(host, std::to_string(port));
	boost::asio::connect(socket_, results);
}

void MetricsImpl::shutdown() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
	socket_.close(ec);
}

void MetricsImpl::increment(const char* key, std::intmax_t value) {
	auto format = std::format("{}:{}|c", key, value);
	send(std::move(format));
}

void MetricsImpl::timing(const char* key, const std::chrono::milliseconds& value) {
	auto format = std::format("{}:{}|ms", key, value.count());
	send(std::move(format));
}

void MetricsImpl::gauge(const char* key, std::uintmax_t value, Adjustment adjustment) {
	std::stringstream format;
	format << key << ":";

	switch (adjustment) {
		case Adjustment::positive: format << "+";
			break;
		case Adjustment::negative: format << "-";
			break;
		case Adjustment::none:
			break;
	}

	format << value << "|g";

	send(format.str());
}

void MetricsImpl::set(const char* key, std::intmax_t value) {
	auto format = std::format("{}:{}|s", key, value);
	send(std::move(format));
}

void MetricsImpl::send(std::string message) {
	auto datagram = std::make_unique<std::string>(std::move(message));
	auto buffer = boost::asio::buffer(*datagram);
	socket_.async_send(buffer,
		[dg = std::move(datagram)](const boost::system::error_code&, std::size_t) { });
}

} // ember