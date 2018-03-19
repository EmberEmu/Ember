/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

enum class InventoryType {
	NON_EQUIP       = 0,
	NVTYPE_HEAD     = 1,
	NECK            = 2,
	SHOULDERS       = 3,
	BODY            = 4,
	CHEST           = 5,
	WAIST           = 6,
	LEGS            = 7,
	FEET            = 8,
	WRISTS          = 9,
	HANDS           = 10,
	FINGER          = 11,
	TRINKET         = 12,
	WEAPON          = 13,
	SHIELD          = 14,
	RANGED          = 15,
	CLOAK           = 16,
	TWOHAND_WEAPON  = 17,
	BAG             = 18,
	TABARD          = 19,
	ROBE            = 20,
	WEAPON_MAINHAND = 21,
	WEAPON_OFFHAND  = 22,
	HOLDABLE        = 23,
	AMMO            = 24,
	THROWN          = 25,
	RANGED_RIGHT    = 26,
	QUIVER          = 27,
	RELIC           = 28
};

} // ember