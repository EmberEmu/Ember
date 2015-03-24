/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <cstdint>

namespace ember {

struct Realm {
	std::uint32_t id;
	std::string name, ip;
	float population;
	std::uint32_t icon;
	std::uint8_t flags;
	std::uint8_t timezone;
};

} //ember