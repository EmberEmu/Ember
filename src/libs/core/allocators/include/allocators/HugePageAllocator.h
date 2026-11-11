/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember::allocators {

class HugePageAllocator {
public:
	void* allocate(std::size_t size);
	void deallocate(void* ptr, std::size_t);
};

} // allocators, ember