/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <array>

namespace ember {

class AsioPools final {
public:
	struct PoolConfig {
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
	enum PoolSize {
		small,
		medium,
		large,
		xlarge,
		huge
	};

	constexpr static auto small_chunks  = 8;
	constexpr static auto medium_chunks = 8;
	constexpr static auto large_chunks  = 8;
	constexpr static auto xlarge_chunks = 4;
	constexpr static auto huge_chunks   = 1;

	constexpr static auto s_small  = 64;
	constexpr static auto s_medium = 128;
	constexpr static auto s_large  = 256;
	constexpr static auto s_xlarge = 512;
	constexpr static auto s_huge = 1024;

	std::array<boost::pool<>, 5> pools {
		boost::pool<>{ s_small,  small_chunks  },
		boost::pool<>{ s_medium, medium_chunks },
		boost::pool<>{ s_large,  large_chunks  },
		boost::pool<>{ s_xlarge, xlarge_chunks },
		boost::pool<>{ s_huge,   huge_chunks   }
	};

	std::array<boost::pool<>, 5> init_pools(const PoolConfig& conf) {
		return {
			boost::pool<>{ conf.small_size,  small_chunks  },
			boost::pool<>{ conf.medium_size, medium_chunks },
			boost::pool<>{ conf.large_size,  large_chunks  },
			boost::pool<>{ conf.xlarge_size, xlarge_chunks },
			boost::pool<>{ conf.huge_size,   huge_chunks   }
		};
	}

public:
	AsioPools(const PoolConfig& conf)
		: pools(init_pools(conf)) {}

	AsioPools() = default;

	inline boost::pool<>* select(const std::size_t size) {
		if(size <= small) {
			return &pools[small];
		} else if(size <= medium) {
			return &pools[medium];
		} else if(size <= large) {
			return &pools[large];
		} else if(size <= xlarge) {
			return &pools[xlarge];
		} else if(size <= huge) {
			return &pools[huge];
		} else {
			return nullptr;
		}
	}
};

} // ember