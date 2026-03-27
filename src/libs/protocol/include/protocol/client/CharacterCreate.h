/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
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
	
	StreamResult read_from_stream(le_stream auto& stream) {
		stream >> spark::io::null_terminated(character.name);
		stream >> character.race;
		stream >> character.class_;
		stream >> character.gender;
		stream >> character.skin;
		stream >> character.face;
		stream >> character.hairstyle;
		stream >> character.haircolour;
		stream >> character.facialhair;
		stream >> character.outfit_id;
		return stream? StreamResult::success : StreamResult::stream_error;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << spark::io::null_terminated(character.name);
		stream << character.race;
		stream << character.class_;
		stream << character.gender;
		stream << character.skin;
		stream << character.face;
		stream << character.hairstyle;
		stream << character.haircolour;
		stream << character.facialhair;
		stream << character.outfit_id;
		return stream? StreamResult::success : StreamResult::stream_error;
	}
};

} // client, protocol, ember