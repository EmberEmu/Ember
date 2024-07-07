/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::spark {

template<typename T>
struct DefaultAllocator final {
	inline T* allocate() const {
		return new T;
	}

	inline void deallocate(T* t) const {
		delete t;
	}
};

} // spark, ember