/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <Character_generated.h>
#include <shared/database/objects/Character.h>
#include <stdexcept>
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

struct CharacterCreate final {
	rpc::Character::CharacterTemplateT character;
	
	StreamResult read_from_stream(auto& stream) try {
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
		return stream? StreamResult::SUCCESS : StreamResult::STREAM_ERROR;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}

	StreamResult write_to_stream(auto& stream) const try {
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
		return stream? StreamResult::SUCCESS : StreamResult::STREAM_ERROR;
	} catch(const std::exception&) {
		return StreamResult::CAUGHT_EXCEPTION;
	}
};

} // client, protocol, ember