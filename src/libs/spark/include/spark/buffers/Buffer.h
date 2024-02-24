/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferRead.h>
#include <spark/buffers/BufferWrite.h>

namespace ember::spark {

class Buffer : public BufferRead, public BufferWrite {
public:
	using BufferRead::operator[];
	using BufferWrite::operator[];

	virtual ~Buffer() = default;
};

} // spark, ember