/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <cstdint>

namespace ember {

struct Vector {
	double x, y, z;
};

struct Character {
	std::string name;
	std::uint32_t id;
	std::uint32_t account_id;
	std::uint32_t realm_id;
	std::uint8_t race;
	std::uint8_t class_;
	std::uint8_t gender;
	std::uint8_t skin;
	std::uint8_t face;
	std::uint8_t hairstyle;
	std::uint8_t haircolour;
	std::uint8_t facialhair;
	std::uint8_t level;
	std::uint32_t zone;
	std::uint32_t map;
	std::uint32_t guild_id;
	std::uint32_t guild_rank;
	Vector position;
	Vector orientation;
	std::uint32_t flags;
	bool first_login;
	std::uint32_t pet_display;
	std::uint32_t pet_level;
	std::uint32_t pet_family;
};

} //ember