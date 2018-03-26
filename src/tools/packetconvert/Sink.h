/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketLog_generated.h"

namespace ember {

class Sink {
public:
	virtual void handle(const fblog::Header& header) = 0;
	virtual void handle(const fblog::Message& message) = 0;

	virtual ~Sink() = default;
};

} // ember