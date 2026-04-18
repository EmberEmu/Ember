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
#include <thread/ServicePool.h>
#include <boost/asio/post.hpp>

namespace ember::realm {

struct ClientDeleter final {
	ClientAllocator& allocator;
	thread::ServicePool& pool;

	explicit ClientDeleter(ClientAllocator& allocator, thread::ServicePool& pool) noexcept
		: allocator(allocator)
		, pool(pool) {}

	void operator()(Client* ptr) const {
		auto& ioc = pool.get(ptr->uuid().service());

		boost::asio::post(ioc, [&, ptr] {
			allocator.deallocate(ptr);
		});
	}
};

using unique_client_ptr = std::unique_ptr<Client, ClientDeleter>;

} // realm, ember