/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientHandlerBuilder.h"
#include "ClientConnectionBuilder.h"
#include "unique_client_ptr.h"
#include <memory>

namespace ember::realm {

class ClientBuilder {
	constexpr static auto allocator_tag = "client_allocator";

	mutable ClientAllocator allocator_;
	ClientHandlerBuilder ch_builder_;
	ClientConnectionBuilder cc_builder_;

	unique_client_ptr make_unique_client(tcp_socket socket, std::size_t index) const {
		return unique_client_ptr(allocator_.allocate(
			ch_builder_, cc_builder_, std::move(socket), index
		), ClientDeleter(&allocator_));
	}

public:
	ClientBuilder(ClientHandlerBuilder ch_builder, ClientConnectionBuilder cc_builder)
		: allocator_(allocator_tag)
		, ch_builder_(ch_builder)
		, cc_builder_(cc_builder) {}

	unique_client_ptr create(tcp_socket socket, std::size_t index) const {
		return make_unique_client(std::move(socket), index);
	}
};

} // realm, ember
