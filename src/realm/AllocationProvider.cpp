/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AllocationProvider.h"

namespace ember::realm {

AllocationProvider::AllocationProvider(std::size_t client_elements, std::size_t node_elements)
	: client_elements_(client_elements)
	, node_elements_(node_elements) {}

auto AllocationProvider::make_buffer() const -> DynamicTLSBuffer {
	return DynamicTLSBuffer {
		DynamicTLSBuffer::allocator_type(node_elements_, buffer_alloc_tag)
	};
}

ClientAllocator& AllocationProvider::client_allocator() const {
	thread_local ClientAllocator instance {
		client_elements_, client_alloc_tag
	};

	return instance;
}

auto AllocationProvider::make_buffer_pair() const -> std::pair<DynamicTLSBuffer, DynamicTLSBuffer> {
	return std::make_pair(
		make_buffer(), make_buffer()
	);
}

} // realm, ember