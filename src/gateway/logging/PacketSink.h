/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Buffer.h>
#include <cstdint>
#include <cstddef>

namespace ember {

class PacketSink {
public:
	virtual void log(const spark::Buffer& buffer, std::size_t length, std::uint64_t timestamp) = 0;

	virtual ~PacketSink() = default;
};

} // ember