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

namespace ember { namespace protocol {

namespace be = boost::endian;

class SMSG_CHAR_ENUM final : public Packet {

	State state_ = State::INITIAL;


public:
	std::vector<Character> characters;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		// todo, like everything else in this file

		return state_;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << std::uint8_t(characters.size());

		for(auto& c : characters) {
			stream << std::uint64_t(c.id());
			stream << c.name();
			stream << c.race();
			stream << c.class_temp();
			stream << c.gender();
			stream << c.skin();
			stream << c.face();
			stream << c.hairstyle();
			stream << c.haircolour();
			stream << c.facialhair();
			stream << c.level();
			stream << c.zone();
			stream << c.map();
			stream << c.x();
			stream << c.y();
			stream << c.z();
			stream << c.guild_id();
			stream << c.flags();
			stream << c.first_login();
			stream << c.pet_display();
			stream << c.pet_level();
			stream << c.pet_family();

			//	// inventory
			//	for(int i = 0; i < 19; ++i) {
			//		stream << std::uint32_t(0);
			//		stream << std::uint8_t(0);
			//	}

			//	stream << std::uint32_t(0);
			//	stream << std::uint8_t(0);

			char arr[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0x27, 0x0, 0x0,
			               0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x27, 0x0, 0x0, 0x7, 0x0, 0x0, 0x0,
			               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcd,
			               0x36, 0x0, 0x0, 0x15, 0x2a, 0x49, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			               0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
			stream.put(arr, sizeof(arr));
		}
	}
};

}} // protocol, ember