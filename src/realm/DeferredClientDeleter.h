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
	boost::asio::io_context& ioc;

	explicit DeferredClientDeleter(ClientAllocator& allocator, boost::asio::io_context& ioc) noexcept
		: allocator(allocator)
		, ioc(ioc) {}

	void operator()(Client* ptr) const {
		ClientSlots::free_slot();

		boost::asio::post(ioc, [&, ptr]()  {
			allocator.deallocate(ptr);
		});
	}
};

using unique_deferred_client = std::unique_ptr<Client, DeferredClientDeleter>;

} // realm, ember