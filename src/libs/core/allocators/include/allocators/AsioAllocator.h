/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PoolAllocator.h"
#include "StaticAllocator.h"
#include <allocators/AllocTrack.h>
#include <string_view>
#include <cstdlib>

namespace ember::allocators {

template<typename T, typename Allocator>
class AsioAllocator {
	template<typename, typename> friend class AsioAllocator;
	Allocator& allocator_;

public:
	static constexpr std::string_view tag { "asio_allocator" };

	using value_type = T;

	explicit AsioAllocator(Allocator& allocator)
		: allocator_(allocator) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	~AsioAllocator() {
		ALLOC_TRACK(tag, mem_rep_destroy);
	}

	template<typename U, typename V>
	AsioAllocator(const AsioAllocator<U, V>& other) noexcept
		: allocator_(other.allocator_) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	bool operator==(const AsioAllocator& other) const noexcept {
		return &allocator_ == &other.allocator_;
	}

	bool operator!=(const AsioAllocator& other) const noexcept {
		return &allocator_ != &other.allocator_;
	}

	value_type* allocate(std::size_t n) const {
		ALLOC_TRACK(tag, mem_rep_alloc);
		ALLOC_TRACK(tag, mem_rep_bytes_alloc, sizeof(value_type) * n);
		return static_cast<value_type*>(allocator_.allocate(sizeof(value_type) * n));
	}

	void deallocate(value_type* ptr, std::size_t n) const {
		ALLOC_TRACK(tag, mem_rep_dealloc);
		ALLOC_TRACK(tag, mem_rep_bytes_dealloc, sizeof(value_type) * n);
		allocator_.deallocate(ptr, sizeof(value_type) * n);
	}
};

} // allocators, ember