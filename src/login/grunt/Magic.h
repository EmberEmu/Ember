/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

 /* This file isn't as exciting as the name implies */

namespace ember { namespace grunt {

// todo - use constexpr func when switched to VS2015 - this is implementation defined behaviour!
enum class Game : std::uint32_t {
	WoW = 'WoW'
};

enum class Platform : std::uint32_t {
	x86 = 'x86',
	PPC = 'PPC'
};

enum class System : std::uint32_t {
	Windows = 'Win',
	OSX = 'OSX'
};

enum Locale : std::uint32_t {
	enGB = 'enGB', enUS = 'enUS',
	esMX = 'esMX', ptBR = 'ptBR',
	frFR = 'frFR', deDE = 'deDE',
	esES = 'esES', ptPT = 'ptPT',
	itIT = 'itIT', ruRU = 'ruRU',
	koKR = 'koKR', zhTW = 'zhTW',
	enTW = 'enTW', enCN = 'enCN',
};

}} // grunt, ember