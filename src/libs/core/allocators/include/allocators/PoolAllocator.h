/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/AllocTrack.h>
#include <boost/pool/pool.hpp>
#include <array>
#include <cstddef>

namespace ember::allocators {

class PoolAllocator final {
public:
	struct Config {
		std::size_t small_size;
		std::size_t small_chunks;
		std::size_t medium_size;
		std::size_t medium_chunks;
		std::size_t large_size;
		std::size_t large_chunks;
		std::size_t xlarge_size;
		std::size_t xlarge_chunks;
		std::size_t huge_size;
		std::size_t huge_chunks;
	};

private:
	static constexpr std::string_view default_tag { "asio_pool_allocator" };
	std::string_view tag;

	constexpr static auto small_chunks  = 8;
	constexpr static auto medium_chunks = 8;
	constexpr static auto large_chunks  = 8;
	constexpr static auto xlarge_chunks = 4;
	constexpr static auto huge_chunks   = 1;

	constexpr static auto s_small  = 64;
	constexpr static auto s_medium = 128;
	constexpr static auto s_large  = 256;
	constexpr static auto s_xlarge = 512;
	constexpr static auto s_huge   = 1024;

	std::array<boost::pool<>, 5> pools {
		boost::pool<>{ s_small,  small_chunks  },
		boost::pool<>{ s_medium, medium_chunks },
		boost::pool<>{ s_large,  large_chunks  },
		boost::pool<>{ s_xlarge, xlarge_chunks },
		boost::pool<>{ s_huge,   huge_chunks   }
	};

	std::array<boost::pool<>, 5> init_pools(const Config& conf) const {
		return {
			boost::pool<>{ conf.small_size,  conf.small_chunks  },
			boost::pool<>{ conf.medium_size, conf.medium_chunks },
			boost::pool<>{ conf.large_size,  conf.large_chunks  },
			boost::pool<>{ conf.xlarge_size, conf.xlarge_chunks },
			boost::pool<>{ conf.huge_size,   conf.huge_chunks   }
		};
	}

	inline boost::pool<>* select(const std::size_t size) {
		for(auto& pool : pools) {
			if(size <= pool.get_requested_size()) {
				return &pool;
			}
		}

		return nullptr;
	}

public:
	PoolAllocator(std::string_view tag = default_tag)
		: tag(tag) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	PoolAllocator(const Config& conf, std::string_view tag = default_tag)
		: tag(tag)
		, pools(init_pools(conf)) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	~PoolAllocator() {
		ALLOC_TRACK(tag, mem_rep_destroy);
	}

	void* allocate(std::size_t size) {
		ALLOC_TRACK(tag, mem_rep_bytes_alloc, size);
		auto pool = select(size);

		if(pool) {
			ALLOC_TRACK(tag, mem_rep_alloc);
			return pool->malloc();
		} else {
			ALLOC_TRACK(tag, mem_rep_system_alloc);
			return std::malloc(size);
		}
	}

	void deallocate(void* ptr, std::size_t size) {
		ALLOC_TRACK(tag, mem_rep_bytes_dealloc, size);
		auto pool = select(size);

		if(pool) {
			ALLOC_TRACK(tag, mem_rep_dealloc);
			pool->free(ptr);
		} else {
			ALLOC_TRACK(tag, mem_rep_system_dealloc);
			std::free(ptr);
		}
	}
};

} // allocators, ember