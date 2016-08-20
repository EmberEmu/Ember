/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/enum_flags.h>
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

	enum class Flag : std::uint8_t {
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

	enum class Type : std::uint32_t {
		PvE, PvP, RP = 6, RPPvP = 8,
	};

	enum class Zone : std::uint8_t { // these are probably wrong
		ANY,
		UNITED_States,
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
	Flag flags;
	Zone zone;
	CreationSetting creation_setting;
};

ENUM_FLAGS(Realm::Flag, std::uint8_t);
ENUM_FLAGS(Realm::Type, std::uint32_t);
ENUM_FLAGS(Realm::Zone, std::uint8_t);
ENUM_FLAGS(Realm::CreationSetting, std::uint8_t);

} //ember