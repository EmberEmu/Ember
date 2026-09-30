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
#include "unique_client_ptr.h"
#include <logger/LoggerFwd.h>
#include <memory>

namespace ember::realm {

class ClientBuilder {
	ClientHandlerBuilder handler_builder_;
	ClientConnectionBuilder conn_builder_;
	const AllocationProvider& alloc_provider_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;

	unique_client_ptr make_unique_client(tcp_socket socket, boost::asio::io_context& ioc) const {
		auto& allocator = alloc_provider_.client_allocator();
		allocator.thread_enter();

		return unique_client_ptr(allocator .allocate(
			handler_builder_.create(ioc.get_executor()),
			conn_builder_.create(std::move(socket)),
			dispatcher_, logger_
		), ClientDeleter(allocator, ioc));
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

	unique_client_ptr create(tcp_socket socket, boost::asio::io_context& ioc) const {
		return make_unique_client(std::move(socket), ioc);
	}
};

} // realm, ember
