/*
 * Copyright (c) 2023 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Transport.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::stun {

enum Protocol {
	UDP, TCP, TLS_TCP
};

class Client {
	enum class State {
		INITIAL, CONNECTING, CONNECTED, DISCONNECTED
	} state_ = State::INITIAL;

	std::unique_ptr<Transport> transport_;
	bool connected_ = false;

public:
	void connect(const std::string& host, std::uint16_t port, Protocol protocol);
	std::string external_address();
};

} // stun, ember