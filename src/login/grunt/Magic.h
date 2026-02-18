/*
 * Copyright (c) 2015 - 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/MulticharConstant.h>
#include <shared/smartenum.hpp>
#include <cstdint>

/* This file isn't as exciting as the name implies */

namespace ember::grunt {

smart_enum_class(Game, std::uint32_t,
	WoW = utility::make_mcc("WoW")
);

smart_enum_class(Platform, std::uint32_t,
	x86 = utility::make_mcc("x86"),
	PPC = utility::make_mcc("PPC")
);

smart_enum_class(System, std::uint32_t,
	Win = utility::make_mcc("Win"),
	OSX = utility::make_mcc("OSX")
);

smart_enum_class(Locale, std::uint32_t,
	enGB = utility::make_mcc("enGB"), enUS = utility::make_mcc("enUS"),
	esMX = utility::make_mcc("esMX"), ptBR = utility::make_mcc("ptBR"),
	frFR = utility::make_mcc("frFR"), deDE = utility::make_mcc("deDE"),
	esES = utility::make_mcc("esES"), ptPT = utility::make_mcc("ptPT"),
	itIT = utility::make_mcc("itIT"), ruRU = utility::make_mcc("ruRU"),
	koKR = utility::make_mcc("koKR"), zhTW = utility::make_mcc("zhTW"),
	enTW = utility::make_mcc("enTW"), enCN = utility::make_mcc("enCN")
);

} // grunt, ember
