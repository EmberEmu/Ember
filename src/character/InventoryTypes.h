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
	non_equip       = 0,
	nvtype_head     = 1,
	neck            = 2,
	shoulders       = 3,
	body            = 4,
	chest           = 5,
	waist           = 6,
	legs            = 7,
	feet            = 8,
	wrists          = 9,
	hands           = 10,
	finger          = 11,
	trinket         = 12,
	weapon          = 13,
	shield          = 14,
	ranged          = 15,
	cloak           = 16,
	twohand_weapon  = 17,
	bag             = 18,
	tabard          = 19,
	robe            = 20,
	weapon_mainhand = 21,
	weapon_offhand  = 22,
	holdable        = 23,
	ammo            = 24,
	thrown          = 25,
	ranged_right    = 26,
	quiver          = 27,
	relic           = 28
};

} // ember