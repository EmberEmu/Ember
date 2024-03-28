/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember::spark::v2 {

struct Header {
	std::uint32_t size;
	std::uint8_t channel;
	std::uint8_t padding;
};

enum Mode {
	CLIENT, SERVER
};

} // v2, spark, ember