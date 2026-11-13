/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Client.h"
#include "ClientAllocator.h"
#include "ClientSlots.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace ember::realm {

struct DeferredClientDeleter final {
	ClientAllocator& allocator;
	Executor executor;

	explicit DeferredClientDeleter(ClientAllocator& allocator, Executor& executor) noexcept
		: allocator(allocator)
		, executor(executor) {}

	void operator()(Client* ptr) const {
		ClientSlots::free_slot();

		boost::asio::post(executor, [allocator = &allocator, ptr]() {
			allocator->destroy(ptr);
			allocator->deallocate(ptr);
		});
	}
};

using unique_deferred_client = std::unique_ptr<Client, DeferredClientDeleter>;

} // realm, ember