/*
 * Copyright (c) 2023 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/DatagramTransport.h>

namespace ember::stun {

DatagramTransport::DatagramTransport(const std::string& host, std::uint16_t port)
	: host_(host), port_(port), socket_(ctx_, ba::ip::udp::endpoint(ba::ip::udp::v4(), 0)) { }

void DatagramTransport::connect() {
	ba::ip::udp::resolver resolver(ctx_);
	ba::ip::udp::resolver::query query(host_, std::to_string(port_));
	boost::asio::connect(socket_, resolver.resolve(query));
}

} // stun, ember