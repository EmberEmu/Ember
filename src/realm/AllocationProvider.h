/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ConnectionDefines.h"
#include "ClientAllocator.h"
#include <string_view>
#include <cstddef>

namespace ember::realm {

class AllocationProvider {
	constexpr static std::string_view client_alloc_tag { "realm_client"  };
	constexpr static std::string_view buffer_alloc_tag { "realm_client_buffers" };

	std::size_t client_elements_;
	std::size_t node_elements_;

public:
	AllocationProvider(std::size_t client_elements, std::size_t node_elements);

	ClientAllocator& client_allocator() const;
	DynamicTLSBuffer make_buffer() const;
	std::pair<DynamicTLSBuffer, DynamicTLSBuffer> make_buffer_pair() const;
};

} // realm, ember