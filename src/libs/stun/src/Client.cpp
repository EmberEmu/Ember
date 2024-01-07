/*
 * Copyright (c) 2023 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Client.h>
#include <stun/TransportUDP.h>
#include <stdexcept>

namespace ember::stun {

void Client::connect(const std::string& host, std::uint16_t port, Protocol protocol) {
	transport_.reset();

	switch (protocol) {
		case Protocol::UDP:
			transport_ = std::make_unique<TransportUDP>(host, port);
			break;
		case Protocol::TCP:
		case Protocol::TLS_TCP:
			throw std::runtime_error("Not handled yet");
	}

	transport_->connect();
}

std::string Client::external_address() {
	return "todo";
}

} // stun, ember