/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <shared/database/objects/Character.h>
#include <boost/endian/conversion.hpp>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

namespace be = boost::endian;

class SMSG_CHAR_ENUM final : public ServerPacket {
	State state_ = State::INITIAL;


public:
	SMSG_CHAR_ENUM() : ServerPacket(protocol::ServerOpcodes::SMSG_CHAR_ENUM) { }

	std::vector<Character> characters;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		be::little_uint8_t char_count;
		stream >> char_count;

		for(auto i = 0; i < char_count; ++i) {
			Character c;
			stream >> c.id;
			stream >> c.name;
			stream >> c.race;
			stream >> c.class_;
			stream >> c.gender;
			stream >> c.skin;
			stream >> c.face;
			stream >> c.hairstyle;
			stream >> c.haircolour;
			stream >> c.facialhair;
			stream >> c.level;
			stream >> c.zone;
			stream >> c.map;
			stream >> c.position.x;
			stream >> c.position.y;
			stream >> c.position.z;
			stream >> c.guild_id;
			stream >> c.flags;
			stream >> c.first_login;
			stream >> c.pet_display;
			stream >> c.pet_level;
			stream >> c.pet_family;

			be::little_to_native_inplace(c.id);
			be::little_to_native_inplace(c.zone);
			be::little_to_native_inplace(c.map);
			be::little_to_native_inplace(c.position.x);
			be::little_to_native_inplace(c.position.y);
			be::little_to_native_inplace(c.position.z);
			be::little_to_native_inplace(c.guild_id);
			be::little_to_native_inplace(c.flags);
			be::little_to_native_inplace(c.pet_display);
			be::little_to_native_inplace(c.pet_level);
			be::little_to_native_inplace(c.pet_family);

			stream.skip(100); // temp, obviously

			characters.emplace_back(std::move(c));
		}


		return state_;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << std::uint8_t(characters.size());

		for(auto& c : characters) {
			stream << be::native_to_little(c.id);
			stream << c.name;
			stream << c.race;
			stream << c.class_;
			stream << c.gender;
			stream << c.skin;
			stream << c.face;
			stream << c.hairstyle;
			stream << c.haircolour;
			stream << c.facialhair;
			stream << c.level;
			stream << be::native_to_little(c.zone);
			stream << c.map;
			stream << be::native_to_little(c.position.x);
			stream << be::native_to_little(c.position.y);
			stream << be::native_to_little(c.position.z);
			stream << be::native_to_little(c.guild_id);
			stream << be::native_to_little(c.flags);
			stream << static_cast<std::uint8_t>(c.first_login);
			stream << be::native_to_little(c.pet_display);
			stream << be::native_to_little(c.pet_level);
			stream << be::native_to_little(c.pet_family);

			unsigned char arr[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0x27, 0x0, 0x0,
			                        0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x27, 0x0, 0x0, 0x7, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcd,
			                        0x36, 0x0, 0x0, 0x15, 0x2a, 0x49, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
			stream.put(arr, sizeof(arr));
		}
	}
};

} // protocol, ember
