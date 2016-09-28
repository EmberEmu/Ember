/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/enum_bitmask.h>
#include <string>
#include <cstdint>

namespace ember {

struct Realm {
	enum class CreationSetting : std::uint8_t {
		ENABLED,
		DISABLED,
		QUEUED,
		EXISTING_ONLY
	};

	enum class Flags : std::uint8_t {
		NONE         = 0x00,
		INVALID      = 0x01,
		OFFLINE      = 0x02,
		SPECIFYBUILD = 0x04,
		UNK1         = 0x08,
		UNK2         = 0x10,
		RECOMMENDED  = 0x20, // can set manually or allow client to do so by setting the population to 600.0f
		NEW_PLAYERS  = 0x40, // can set manually or allow client to do so by setting the population to 200.0f
		FULL         = 0x80  // can set manually or allow client to do so by setting the population to 400.0f
	}; 

	enum class Type : std::uint32_t {
		PvE, PvP, RP = 6, RPPvP = 8,
	};

	enum class Zone : std::uint8_t { // these are probably wrong
		ANY,
		UNITED_STATES,
		KOREA,
		ENGLISH,
		TAIWAN,
		CHINA,
		TEST_SERVER = 99,
		QA_SERVER = 101,
	};

	std::uint32_t id;
	std::string name, ip;
	float population;
	Type type;
	Flags flags;
	Zone zone;
	CreationSetting creation_setting;
};

ENABLE_BITMASK(Realm::Flags);
ENABLE_BITMASK(Realm::Type);
ENABLE_BITMASK(Realm::Zone);
ENABLE_BITMASK(Realm::CreationSetting);

} //ember