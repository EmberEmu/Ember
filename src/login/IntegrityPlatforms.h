/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <string_view>

using namespace std::string_view_literals;

namespace ember {

inline const std::array winx86 {
	"WoW.exe"sv,
	"fmod.dll"sv,
	"ijl15.dll"sv,
    "dbghelp.dll"sv,
	"unicows.dll"sv
};

inline const std::array macx86 {
	"MacOS/World of Warcraft"sv,
	"Info.plist"sv,
    "Resources/Main.nib/objects.xib"sv,
	"Resources/wow.icns"sv,
	"PkgInfo"sv
};

inline const std::array macppc {
	"MacOS/World of Warcraft"sv,
	"Info.plist"sv,
    "Resources/Main.nib/objects.xib"sv,
    "Resources/wow.icns"sv,
	"PkgInfo"sv 
};

} // ember