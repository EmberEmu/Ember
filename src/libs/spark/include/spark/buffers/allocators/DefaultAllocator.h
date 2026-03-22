/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <utility>

namespace ember::spark::io {

template<typename T>
struct DefaultAllocator final {
	const char* tag;

#ifdef EMBER_DEBUG_ALLOCATORS
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
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

	DefaultAllocator(const char* tag = nullptr)	: tag(tag) {}

	~DefaultAllocator() {
#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
	}
};

} // io, spark, ember