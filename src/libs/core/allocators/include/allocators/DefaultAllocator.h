/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/AllocTrack.h>
#include <string_view>
#include <utility>

namespace ember::allocators {

template<typename T>
struct DefaultAllocator final {
	std::string_view tag;

#ifdef EMBER_DEBUG_ALLOCATORS
	mutable std::size_t active_count = 0;
	mutable std::size_t total_allocs = 0;
	mutable std::size_t total_deallocs = 0;
#endif

	template<typename ...Args>
	[[nodiscard]] inline T* allocate(Args&&... args) const {
#ifdef EMBER_DEBUG_ALLOCATORS
		++active_count;
		++total_allocs;
#endif
		return new T(std::forward<Args>(args)...);
	}

	inline void deallocate(T* t) const {
#ifdef EMBER_DEBUG_ALLOCATORS
		--active_count;
		++total_deallocs;
#endif
		delete t;
	}

	DefaultAllocator(std::string_view tag = {}) : tag(tag) {}

	DefaultAllocator(DefaultAllocator&& rhs) noexcept
		: tag(rhs.tag)
#ifdef EMBER_DEBUG_ALLOCATORS
		, active_count(rhs.active_count)
		, total_allocs(rhs.total_allocs)
		, total_deallocs(rhs.total_deallocs) 
#endif
	{
		rhs.tag = "";
#ifdef EMBER_DEBUG_ALLOCATORS
		rhs.active_count = 0;
		rhs.total_allocs = 0;
		rhs.total_deallocs = 0;
#endif
	}
	
	DefaultAllocator& operator=(DefaultAllocator&& rhs) noexcept {
		tag = rhs.tag;
#ifdef EMBER_DEBUG_ALLOCATORS
		active_count = rhs.active_count;
		total_allocs = rhs.total_allocs;
		total_deallocs = rhs.total_deallocs;
		rhs.active_count = 0;
		rhs.total_allocs = 0;
		rhs.total_deallocs = 0;
#endif
		return *this;
	}

	DefaultAllocator(DefaultAllocator& rhs) = default;
	DefaultAllocator& operator=(DefaultAllocator& rhs) = default;

	~DefaultAllocator() {
#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
	}
};

} // allocators, ember