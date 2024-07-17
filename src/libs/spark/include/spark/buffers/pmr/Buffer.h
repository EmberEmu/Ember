/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BufferRead.h>
#include <spark/buffers/pmr/BufferWrite.h>

namespace ember::spark::io::pmr {

class Buffer : public BufferRead, public BufferWrite {
public:
	using BufferRead::operator[];

	virtual ~Buffer() = default;
};

} // pmr, io, spark, ember