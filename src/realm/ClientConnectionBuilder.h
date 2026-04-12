/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"

namespace ember::realm {

class ClientConnectionBuilder final {
	log::Logger& logger_;

public:
	ClientConnectionBuilder(log::Logger& logger)
		: logger_(logger) {}

	ClientConnection create(tcp_socket socket) const {
		return ClientConnection(std::move(socket), logger_);
	}
};

} // realm, ember
