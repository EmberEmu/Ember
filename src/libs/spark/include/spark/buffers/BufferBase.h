/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember::spark {

template<typename T>
concept byte_oriented = sizeof(T) == 1;

class BufferBase {
public:
	virtual std::size_t size() const = 0;
	virtual bool empty() const = 0;
	virtual std::byte& operator[](const std::size_t index) = 0; // todo, find a way to remove this
	virtual ~BufferBase() = default;
};

} // spark, ember