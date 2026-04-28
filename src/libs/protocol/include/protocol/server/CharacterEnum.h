/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/Packet.h>
#include <protocol/StreamResult.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/StringAdaptors.h>
#include <shared/database/objects/Character.h>
#include <stdexcept>
#include <vector>
#include <cstdint>

namespace ember::protocol::server {

struct CharacterEnum final {
	std::vector<Character> characters;

	StreamResult read_from_stream(le_stream auto& stream) {
		std::uint8_t char_count;
		stream >> char_count;

		for(auto i = 0; i < char_count; ++i) {
			Character c;
			stream >> c.id;
			stream >> spark::io::null_terminated(c.name);
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

			stream.skip(100); // temp, obviously

			characters.emplace_back(std::move(c));
		}

		return stream? StreamResult::success : StreamResult::failed;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << std::uint8_t(characters.size());

		for(auto& c : characters) {
			stream << c.id;
			stream << spark::io::null_terminated(c.name);
			stream << c.race;
			stream << c.class_;
			stream << c.gender;
			stream << c.skin;
			stream << c.face;
			stream << c.hairstyle;
			stream << c.haircolour;
			stream << c.facialhair;
			stream << c.level;
			stream << c.zone;
			stream << c.map;
			stream << c.position.x;
			stream << c.position.y;
			stream << c.position.z;
			stream << c.guild_id;
			stream << c.flags;
			stream << static_cast<std::uint8_t>(c.first_login);
			stream << c.pet_display;
			stream << c.pet_level;
			stream << c.pet_family;

			unsigned char arr[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0x27, 0x0, 0x0,
			                        0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x27, 0x0, 0x0, 0x7, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcd,
			                        0x36, 0x0, 0x0, 0x15, 0x2a, 0x49, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
			stream << arr;
		}

		return stream? StreamResult::success : StreamResult::failed;
	}
};

} // server, protocol, ember

namespace ember::protocol {

using smsg_char_enum = ServerPacket<ServerOpcode::smsg_char_enum, server::CharacterEnum>;

} // protocol, ember