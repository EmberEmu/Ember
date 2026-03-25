/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/DynamicBuffer.h>
#include <spark/buffers/allocators/TLSBlockAllocator.h>
#include <cstddef>

namespace ember::spark::io {

// DynamicBuffer backed by thread-local storage, meaning every
// instance on the same thread shares the same underlying memory.
// As a rule of thumb, an instance should never be touched by any thread
// other than the one on which it was created, not even if synchronised
// ... unless you're positive it won't result in the allocator being called.
// 
// Minimum memory usage is IntrusiveStorage<block_size> * count.
// Additional blocks are not added if the original is exhausted ('colony' structure),
// so the allocator will fall back to the system allocator instead.
//
// Pros: extremely fast allocation/deallocation for many instances per thread
// Cons: everything else.
// 
// TL;DR Do not use unless you know what you're doing.
template<decltype(auto) block_size,
	std::size_t count,
	typename ref_count_policy = NoRefCounting,
	typename entrant_policy = SafeEntrant,
	typename storage_type = std::byte>
using DynamicTLSBuffer = DynamicBuffer<block_size, storage_type,
	TLSBlockAllocator<typename DynamicBuffer<
		block_size>::storage_type, count, NoRefCounting, entrant_policy, spark::io::PageLock
	>
>;

} // io, spark, ember
