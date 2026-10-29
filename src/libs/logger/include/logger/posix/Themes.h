/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/posix/Theme.h>
#include <logger/posix/ThemeUtility.h>
#include <logger/posix/themes/Abyssal.h>
#include <logger/posix/themes/Aurora.h>
#include <logger/posix/themes/Daybreak.h>
#include <logger/posix/themes/Legacy.h>
#include <logger/posix/themes/Midnight.h>
#include <logger/posix/themes/Regalia.h>
#include <logger/posix/themes/Sahara.h>
#include <logger/posix/themes/Smoky.h>
#include <logger/posix/themes/Frost.h>
#include <array>
#include <span>
#include <stdexcept>

namespace ember::log::posix {

inline constexpr std::array builtins {
	themes::Legacy,
	themes::Aurora,
	themes::Daybreak,
	themes::Abyssal,
	themes::Smoky,
	themes::Sahara,
	themes::Regalia,
	themes::Midnight,
	themes::Frost
};

inline Theme builtin_theme(std::string_view name) {
	for(const auto& theme : builtins) {
		if(theme.name == name) {
			return Theme(theme);
		}
	}

	throw std::runtime_error("unknown theme requested");
}

constexpr const std::span<const ThemeView> builtin_themes() {
	return builtins;
}

inline bool has_builtin_theme(std::string_view name) {
	for(const auto& theme : builtins) {
		if(theme.name == name) {
			return true;
		}
	}

	return false;
}

} // posix, log, ember