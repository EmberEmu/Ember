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
	std::unique_ptr<Character> character;
	
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

		character = std::make_unique<Character>(
			name,
			0, // ID
			0, // account ID
			0, // realm ID
			race,
			class_,
			gender,
			skin, 
			face,
			hair_style,
			hair_colour,
			facial_hair,
			0, // level
			0, // zone
			0, // map
			0, // guild ID
			0.f, 0.f, 0.f, // x y z
			0, // flags
			false, // first login,
			0, // pet display
			0, // pet level
			0 // pet family
		);

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