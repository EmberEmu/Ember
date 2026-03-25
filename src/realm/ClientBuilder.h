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
#include <memory>

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
	spark::io::SafeEntrant,
	PageLockPolicy
>;

class ClientBuilder {
	constexpr inline static auto allocator_tag = "client_allocator";

	ClientAllocator allocator_;
	log::Logger& logger_;

public:
	ClientBuilder(log::Logger& logger)
		: allocator_(allocator_tag)
		, logger_(logger) {}

	ClientPtr create(tcp_socket socket, ClientIdent ident) {
		return ClientPtr(
			allocator_.allocate(std::move(socket), std::move(ident), logger_), [&](auto ptr) {
				allocator_.deallocate(ptr);
			}
		);
	}
};

} // realm, ember
