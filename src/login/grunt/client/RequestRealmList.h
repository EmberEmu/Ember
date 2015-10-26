/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class RequestRealmList {
	Opcode opcode;
	std::uint32_t unknown;

public:
};

}}} // client, grunt, ember