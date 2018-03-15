/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"

namespace ember {

class FBLogger final : public PacketSink {
public:
	void log(spark::Buffer& buffer, std::size_t length, std::uint64_t timestamp) override;
};

} // ember