/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::spark::io {

template<typename T>
struct DefaultAllocator final {
	template<typename ...Args>
	[[nodiscard]] inline T* allocate(Args&&... args) const {
		return new T(std::forward<Args>(args)...);
	}

	inline void deallocate(T* t) const {
		delete t;
	}
};

} // io, spark, ember