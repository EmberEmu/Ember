/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferIn.h>
#include <spark/buffers/BufferOut.h>

namespace ember::spark {

class Buffer : public BufferIn, public BufferOut {
public:
	virtual ~Buffer() = default;
};

} // spark, ember