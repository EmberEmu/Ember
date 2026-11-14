/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AllocationProvider.h"
#include "ClientHandlerBuilder.h"
#include "ClientConnectionBuilder.h"
#include "Forwards.h"
#include "DeferredClientDeleter.h"
#include <logger/LoggerFwd.h>
#include <memory>

namespace ember::realm {

class ClientBuilder {
	ClientHandlerBuilder handler_builder_;
	ClientConnectionBuilder conn_builder_;
	const AllocationProvider& alloc_provider_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;

	DeferredClient make_deferred_client(tcp::Socket socket, boost::asio::ip::tcp::endpoint endpoint) const {
		auto& allocator = alloc_provider_.client_allocator();
		auto client = allocator.allocate();
		auto executor = socket.get_executor();

		try {
			std::construct_at(
				client, handler_builder_.create(executor),
				conn_builder_.create(std::move(socket), std::move(endpoint)),
				dispatcher_, logger_
			);
		} catch(...) {
			allocator.deallocate(client);
			throw;
		}

		return DeferredClient(
			client, DeferredClientDeleter(allocator, executor)
		);
	}

public:
	ClientBuilder(ClientHandlerBuilder ch_builder, ClientConnectionBuilder cc_builder,
				  const AllocationProvider& alloc_provider, EventDispatcher& dispatcher,
	              log::Logger& logger)
		: handler_builder_(ch_builder)
		, conn_builder_(cc_builder)
		, alloc_provider_(alloc_provider)
		, dispatcher_(dispatcher)
		, logger_(logger) {}

	DeferredClient create(tcp::Socket socket, boost::asio::ip::tcp::endpoint endpoint) const {
		return make_deferred_client(std::move(socket), std::move(endpoint));
	}
};

} // realm, ember
