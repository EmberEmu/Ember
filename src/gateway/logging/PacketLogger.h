/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"
#include <spark/BinaryStream.h>
#include <spark/Buffer.h>
#include <memory>
#include <vector>

namespace ember {

class PacketLogger final {
	std::vector<std::unique_ptr<PacketSink>> sinks_;

public:
	void add_sink(std::unique_ptr<PacketSink> sink);
	void reset();

	void log(spark::Buffer& buffer, std::size_t length);
};

} // ember