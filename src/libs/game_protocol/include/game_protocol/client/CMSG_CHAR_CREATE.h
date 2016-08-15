/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <shared/database/objects/Character.h>
#include <boost/endian/conversion.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember { namespace protocol {

namespace be = boost::endian;

class CMSG_CHAR_CREATE final : public Packet {
	State state_ = State::INITIAL;

public:
	Character character; // todo, replace this type with CharacterTemplate
	
	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		// todo, everything needs redone
		std::string name;
		std::uint8_t race;
		std::uint8_t class_;
		std::uint8_t gender;
		std::uint8_t skin;
		std::uint8_t face;
		std::uint8_t hair_style;
		std::uint8_t hair_colour;
		std::uint8_t facial_hair;
		std::uint8_t outfit_id;

		stream >> name;
		stream >> race;
		stream >> class_;
		stream >> gender;
		stream >> skin;
		stream >> face;
		stream >> hair_style;
		stream >> hair_colour;
		stream >> facial_hair;
		stream >> outfit_id;

		character.name = name;
		character.id = 0;
		character.account_id = 0;
		character.realm_id = 0;
		character.race = race;
		character.class_ = class_;
		character.gender = gender;
		character.skin = skin;
		character.face = face;
		character.hairstyle = hair_style;
		character.haircolour = hair_colour;
		character.facialhair = facial_hair;
		character.level = 0;
		character.zone = 0;
		character.map = 0;
		character.guild_id = 0;
		character.guild_rank = 0;
		character.position.x = 0.0f;
		character.position.y = 0.0f;
		character.position.z = 0.0f;
		character.flags = 0;
		character.first_login = false;
		character.pet_display = 0;
		character.pet_level = 0;
		character.pet_family = 0;

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		//stream << name;
		//stream << race;
		//stream << class_;
		//stream << gender;
		//stream << skin;
		//stream << face;
		//stream << hair_style;
		//stream << hair_colour;
		//stream << facial_hair;
		//stream << outfit_id;
	}
};

}} // protocol, ember