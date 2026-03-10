/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <string_view>

namespace ember {

inline const std::array<std::string_view, 18> dbcs_required {
	"ChrClasses",
	"ChrRaces",
	"CharBaseInfo",
	"NamesProfanity",
	"NamesReserved",
	"CharSections",
	"CharacterFacialHairStyles",
	"CharStartBase",
	"CharStartSpells",
	"CharStartSkills",
	"CharStartZones",
	"CharStartOutfit",
	"AreaTable",
	"FactionTemplate",
	"FactionGroup",
	"SpamMessages",
	"CharStartOutfit",
	"StartItemQuantities"
};

};