/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <array>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct UpdateObject final {
	std::array<std::uint8_t, 131> data {
		1, 0, 0, 0, // Block Count
		0, // Has transport
		3, // Update type = CREATE_NEW_OBJECT2
		1, 4, // Packed GUID. GUID is 4, mask byte indicates that only the first (least significant) byte is sent.
		4, // Object Type WO_PLAYER
		113, // Update flags 0x71
		0, 0, 0, 0, // Movement flags
		0, 0, 0, 0, // Timestamp
		205, 215, 11, 198, // Position x (-8949.95)
		53, 126, 4, 195, // Position y (-132.493)
		249, 15, 167, 66, // Position z (83.5312) Human start position (Northshire Abbey)
		0, 0, 0, 0, // Orientation (0.0)
		0, 0, 0, 0, // Fall time (0)
		0, 0, 128, 63, // Walk speed (1.0)
		0, 0, 0, 65, // Run speed (70.0)
		0, 0, 144, 64, // Run back speed (4.5)
		0, 0, 0, 0, // Swim speed (0.0)
		0, 0, 0, 0, // Swim back speed (0.0)
		219, 15, 73, 64, // Turn speed (3.1415 (pi))
		1, // Is player
		1, 0, 0, // Unknown hardcoded
		5, // Amount of u32 mask blocks that will follow
		// Mask blocks
		23, 0, 64, 16,    28, 0, 0, 0,    0, 0, 0, 0,    0, 0, 0, 0,    24, 0, 0, 0,
		// End of mask blocks
		4, 0, 0, 0, 0, 0, 0, 0, // OBJECT_FIELD_GUID (4) (notice unpacked u64)
		25, 0, 0, 0, // OBJECT_FIELD_TYPE (16 | 8 | 1) (TYPE_PLAYER | TYPE_UNIT | TYPE_OBJECT)
		0, 0, 128, 63, // Scale (1.0)
		100, 0, 0, 0, // UNIT_FIELD_HEALTH (100)
		100, 0, 0, 0, // UNIT_FIELD_MAXHEALTH (100)
		1, 0, 0, 0, // UNIT_FIELD_LEVEL (1)
		1, 0, 0, 0, // UNIT_FIELD_FACTIONTEMPLATE (1)
		1, // UNIT_FIELD_BYTES[0] // Race (Human)
		1, // UNIT_FIELD_BYTES[1] // Class (Warrior)
		1, // UNIT_FIELD_BYTES[2] // Gender (Female)
		1, // UNIT_FIELD_BYTES[3] // Power (Rage)
		50, 0, 0, 0, // UNIT_FIELD_DISPLAYD (50, Human Female)
		50, 0, 0, 0, // UNIT_FIELD_NATIVEDISPLAYID (50, Human Female)
	};

	StreamResult read_from_stream(le_stream auto& stream) try {
		stream >> data;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(le_stream auto& stream) const try {
		stream << data;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
