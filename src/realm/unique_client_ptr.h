/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Client.h"
#include <spark/buffers/allocators/TLSBlockAllocator.h>

namespace ember::realm {

#ifndef PREALLOCATED_CLIENTS_PER_THREAD
	#define PREALLOCATED_CLIENTS_PER_THREAD 4
#endif

#ifdef ENABLE_PAGE_LOCKING
using PageLockPolicy = spark::io::PageLock;
#else
using PageLockPolicy = spark::io::NoPageLock;
#endif

using ClientAllocator = spark::io::TLSBlockAllocator<
	Client,
	PREALLOCATED_CLIENTS_PER_THREAD,
	spark::io::NoRefCounting,
	spark::io::UnsafeEntrant,
	PageLockPolicy
>;

struct ClientDeleter final {
	ClientAllocator* allocator;

	explicit ClientDeleter(ClientAllocator* allocator) noexcept
		: allocator(allocator) {}

	void operator()(Client* ptr) const {
		if(ptr) {
			allocator->deallocate(ptr);
		}
	}
};

using unique_client_ptr = std::unique_ptr<Client, ClientDeleter>;

} // realm, ember

