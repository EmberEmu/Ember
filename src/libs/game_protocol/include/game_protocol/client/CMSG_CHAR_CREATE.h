/*
 * Copyright (c) 2016 - 2018  Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <shared/database/objects/Character.h>
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

class CMSG_CHAR_CREATE final : public Packet {
	State state_ = State::INITIAL;

public:
	CharacterTemplate character;
	
	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> character.name;
		stream >> character.race;
		stream >> character.class_;
		stream >> character.gender;
		stream >> character.skin;
		stream >> character.face;
		stream >> character.hairstyle;
		stream >> character.haircolour;
		stream >> character.facialhair;
		stream >> character.outfit_id;

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << character.name;
		stream << character.race;
		stream << character.class_;
		stream << character.gender;
		stream << character.skin;
		stream << character.face;
		stream << character.hairstyle;
		stream << character.haircolour;
		stream << character.facialhair;
		stream << character.outfit_id;
	}
};

} // protocol, ember