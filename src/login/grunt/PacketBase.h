/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Buffer.h>

namespace ember { namespace grunt {

struct PacketBase {
	enum class State {
		INITIAL, CALL_AGAIN, DONE
	};

	virtual State deserialise(spark::Buffer& buffer) = 0;
	virtual ~PacketBase() = default;
};

}} // grunt, ember