/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/StreamTransport.h>

namespace ember::stun {

	StreamTransport::StreamTransport(const std::string& host, std::uint16_t port)
	: host_(host), port_(port), socket_(ctx_, ba::ip::udp::endpoint(ba::ip::udp::v4(), 0)) { }

void StreamTransport::connect() {
	ba::ip::udp::resolver resolver(ctx_);
	ba::ip::udp::resolver::query query(host_, std::to_string(port_));
	boost::asio::connect(socket_, resolver.resolve(query));
}

void StreamTransport::send(std::vector<std::uint8_t> message) {

}

} // stun, ember