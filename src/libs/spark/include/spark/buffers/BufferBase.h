/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember::spark {

class BufferBase {
public:
	virtual std::size_t size() const = 0;
	virtual bool empty() const = 0;
	virtual ~BufferBase() = default;
};

} // spark, ember