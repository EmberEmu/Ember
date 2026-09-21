/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember::realm {

struct AllocationConfig {
	std::size_t clients;
	std::size_t nodes;
};

namespace detail {

static inline AllocationConfig config;

} // detail

inline void alloc_cfg(AllocationConfig config) {
	detail::config = config;
}

inline const AllocationConfig& alloc_cfg() {
	return detail::config;
}

} // realm, ember