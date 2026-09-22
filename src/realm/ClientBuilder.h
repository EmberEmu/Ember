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
#include <thread/ServicePool.h>
#include <memory>

namespace ember::realm {

class ClientBuilder {
	ClientHandlerBuilder ch_builder_;
	ClientConnectionBuilder cc_builder_;
	const AllocationProvider& alloc_provider_;
	EventDispatcher& dispatcher_;
	thread::ServicePool& pool_;
	log::Logger& logger_;

	unique_client_ptr make_unique_client(tcp_socket socket, std::size_t index) const {
		auto& allocator = alloc_provider_.client_allocator();
		allocator.thread_enter();

		return unique_client_ptr(allocator .allocate(
			std::move(socket), index, dispatcher_, logger_, ch_builder_, cc_builder_
		), ClientDeleter(allocator, pool_.get(index)));
	}

public:
	ClientBuilder(ClientHandlerBuilder ch_builder, ClientConnectionBuilder cc_builder,
				  const AllocationProvider& alloc_provider, EventDispatcher& dispatcher,
	              thread::ServicePool& pool, log::Logger& logger)
		: ch_builder_(ch_builder)
		, cc_builder_(cc_builder)
		, dispatcher_(dispatcher)
		, alloc_provider_(alloc_provider)
		, pool_(pool)
		, logger_(logger) {}

	unique_client_ptr create(tcp_socket socket, std::size_t index) const {
		return make_unique_client(std::move(socket), index);
	}
};

} // realm, ember
