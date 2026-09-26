/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AllocationProvider.h"
#include "ClientConnection.h"
#include "Forwards.h"

namespace ember::realm {

class ClientConnectionBuilder final {
	const AllocationProvider& alloc_provider_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;

public:
	ClientConnectionBuilder(const AllocationProvider& alloc_provider,
	                        EventDispatcher& dispatcher,
	                        log::Logger& logger)
		: alloc_provider_(alloc_provider)
		, dispatcher_(dispatcher)
		, logger_(logger) {}

	ClientConnection create(tcp_socket socket, const ClientIdent& ident) const {
		return ClientConnection(
			std::move(socket), ident, alloc_provider_.make_buffer_pair(), dispatcher_, logger_
		);
	}
};

} // realm, ember
