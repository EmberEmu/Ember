/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <utility>

namespace ember {

/* TODO, TEMPORARY CODE*/

class Character {
	std::string name_;
	std::uint32_t id_;
	std::uint32_t account_id_;
	std::uint32_t realm_id_;
	std::uint8_t race_;
	std::uint8_t class_;
	std::uint8_t gender_; 
	std::uint8_t skin_;
	std::uint8_t face_;
	std::uint8_t hairstyle_;
	std::uint8_t haircolour_;
	std::uint8_t facialhair_;
	std::uint8_t level_;
	std::uint32_t zone_;
	std::uint32_t map_;
	std::uint32_t guild_id_;
	std::uint32_t guild_rank_;
	float x_, y_, z_;
	std::uint32_t flags_;
	bool first_login_;
	std::uint32_t pet_display_;
	std::uint8_t pet_level_;
	std::uint32_t pet_family_;

public:
	Character(std::string name, std::uint32_t id, std::uint32_t account_id, std::uint32_t realm_id, std::uint8_t race,
	          std::uint8_t class_, std::uint8_t gender, std::uint8_t skin, std::uint8_t face,
	          std::uint8_t hairstyle, std::uint8_t haircolour, std::uint8_t facialhair, std::uint8_t level,
	          std::uint32_t zone, std::uint32_t map, std::uint32_t guild_id, std::uint32_t guild_rank,
	          float x, float y, float z, std::uint32_t flags, bool first_login,
	          std::uint32_t pet_display, std::uint8_t pet_level, std::uint32_t pet_family)
	          : name_(name), account_id_(account_id), realm_id_(realm_id), race_(race), class_(class_), gender_(gender),
	          skin_(skin), face_(face), hairstyle_(hairstyle), haircolour_(haircolour), facialhair_(facialhair), level_(level),
	          zone_(zone), map_(map), x_(x), y_(y), z_(z), flags_(flags), first_login_(first_login), pet_display_(pet_display),
	          pet_level_(pet_level), pet_family_(pet_family), guild_id_(guild_id), guild_rank_(guild_rank), id_(id) { }

	std::uint32_t id() const {
		return id_;
	}

	std::string name() const {
		return name_;
	}

	std::uint32_t account_id() const {
		return account_id_;
	}

	std::uint32_t realm_id() const {
		return realm_id_;
	}

	std::uint8_t race() const {
		return race_;
	}

	std::uint8_t class_temp() const {
		return class_;
	}

	std::uint8_t gender() const {
		return gender_;
	}

	std::uint8_t skin() const {
		return skin_;
	}

	std::uint8_t face() const {
		return face_;
	}

	std::uint8_t hairstyle() const {
		return hairstyle_;
	}

	std::uint8_t haircolour() const {
		return haircolour_;
	}

	std::uint8_t facialhair() const {
		return facialhair_;
	}

	std::uint8_t level() const {
		return level_;
	}

	std::uint32_t zone() const {
		return zone_;
	}

	std::uint32_t map() const {
		return map_;
	}

	std::uint32_t guild_id() const {
		return guild_id_;
	}

	std::uint32_t guild_rank() const {
		return guild_rank_;
	}

	float x() const {
		return x_;
	}

	float y() const{
		return y_;
	}

	float z() const{
		return z_;
	}

	std::uint32_t flags() const {
		return flags_;
	}

	bool first_login() const {
		return first_login_;
	}

	std::uint32_t pet_display() const {
		return pet_display_;
	}

	std::uint32_t pet_level() const {
		return pet_level_;
	}

	std::uint32_t pet_family() const {
		return pet_family_;
	}
};

} //ember