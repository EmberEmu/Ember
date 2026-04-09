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

namespace ember::realm {

struct ClientDeleter final {
	ClientAllocator* allocator;

	explicit ClientDeleter(ClientAllocator* allocator) noexcept
		: allocator(allocator) {}

	void operator()(Client* ptr) const {
		allocator->deallocate(ptr);
	}
};

using unique_client_ptr = std::unique_ptr<Client, ClientDeleter>;

} // realm, ember

