/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AsioPools.h"
#include <cstdlib>

namespace ember {

template <typename T>
class AsioAllocator {
	template <typename> friend class AsioAllocator;
	AsioPools& pools_;

#ifdef ASIO_POOL_ALLOCATOR_DEBUG
	std::size_t pool_allocs { 0 };
	std::size_t glob_allocs { 0 };
#endif

public:
	using value_type = T;

	explicit AsioAllocator(AsioPools& pools)
		: pools_(pools) {}

	template<typename U>
	AsioAllocator(const AsioAllocator<U>& other) noexcept
		: pools_(other.pools_) {}

	bool operator==(const AsioAllocator& other) const noexcept {
		return &pools_ == &other.pools_;
	}

	bool operator!=(const AsioAllocator& other) const noexcept {
		return &pools_ != &other.pools_;
	}

	value_type* allocate(std::size_t n) const {
		auto pool = pools_.select(sizeof(value_type) * n);

		if(pool) [[likely]] {
#ifdef ASIO_POOL_ALLOCATOR_DEBUG
			++pool_allocs;
#endif
			return static_cast<value_type*>(pool->malloc());
		} else {
#ifdef ASIO_POOL_ALLOCATOR_DEBUG
			++glob_allocs;
#endif
			return static_cast<value_type*>(std::malloc(sizeof(value_type) * n));
		}
	}

	void deallocate(value_type* ptr, std::size_t n) const {
		auto pool = pools_.select(sizeof(value_type) * n);

		if(pool) [[likely]] {
			pool->free(ptr);
		} else {
			std::free(ptr);
		}
	}

#ifdef ASIO_POOL_ALLOCATOR_DEBUG
	std::size_t global_allocs() const {
		return glob_allocs;
	}

	std::size_t pool_allocs() const {
		return pool_allocs;
	}
#endif
};

} // ember