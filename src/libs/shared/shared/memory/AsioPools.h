/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/pool/pool.hpp>
#include <array>
#include <cstddef>

namespace ember {

class AsioPools final {
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

public:
	AsioPools() = default;

	AsioPools(const Config& conf)
		: pools(init_pools(conf)) {}

	inline boost::pool<>* select(const std::size_t size) {
		for(auto& pool : pools) {
			if(size <= pool.get_requested_size()) {
				return &pool;
			}
		}

		return nullptr;
	}
};

} // ember