/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/metrics/MetricsImpl.h>
#include <functional>
#include <memory>
#include <sstream>
#include <utility>
#include <cstddef>

namespace ember {

MetricsImpl::MetricsImpl(boost::asio::io_service& service, const std::string& host,
                         std::uint16_t port)
                         : socket_(service), signals_(service, SIGINT, SIGTERM) {
	signals_.async_wait(std::bind(&MetricsImpl::shutdown, this));
	boost::asio::ip::udp::resolver resolver(service);
	boost::asio::ip::udp::resolver::query query(host, std::to_string(port));
	boost::asio::connect(socket_, resolver.resolve(query));
}

void MetricsImpl::shutdown() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
}

void MetricsImpl::increment(const char* key, std::intmax_t value) {
	std::stringstream format;
	format << key << ":" << value << "|c";
	send(format.str());
}

void MetricsImpl::timing(const char* key, std::chrono::milliseconds value) {
	std::stringstream format;
	format << key << ":" << value.count() << "|ms";
	send(format.str());
}

void MetricsImpl::gauge(const char* key, std::uintmax_t value, Adjustment adjustment) {
	std::stringstream format;
	format << key << ":";

	switch (adjustment) {
		case Adjustment::POSITIVE: format << "+";
			break;
		case Adjustment::NEGATIVE: format << "-";
			break;
		case Adjustment::NONE:
			break;
	}

	format << value << "|g";

	send(format.str());
}

void MetricsImpl::set(const char* key, std::intmax_t value) {
	std::stringstream format;
	format << key << ":" << value << "|s";
	send(format.str());
}

void MetricsImpl::send(std::string message) {
	auto datagram = std::make_shared<std::string>(std::move(message));
	socket_.async_send(boost::asio::buffer(*datagram),
		[datagram](const boost::system::error_code&, std::size_t) { });
}

} // ember