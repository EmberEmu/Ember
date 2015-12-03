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
	enum Flag : std::uint8_t {
		NONE         = 0x00,
		INVALID      = 0x01,
		OFFLINE      = 0x02,
		SPECIFYBUILD = 0x04,
		UNK1         = 0x08,
		UNK2         = 0x10,
		NEW_PLAYERS  = 0x20,
		RECOMMENDED  = 0x40,
		FULL         = 0x80
	}; 

	enum Type : std::uint32_t {
		PvE, PvP, RP = 6, RPPvP = 8,
	};

	std::uint32_t id;
	std::string name, ip;
	float population;
	Type type;
	Flag flags;
	std::uint8_t timezone;
};

} //ember