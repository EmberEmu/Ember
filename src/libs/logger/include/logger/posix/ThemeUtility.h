/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/posix/Theme.h>
#include <logger/Severity.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace ember::log::posix {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
	ThemeStorage::StyleBody,
	trace, debug, info, warn, error, fatal, con, conerr
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
	ThemeStorage,
	name, desc, prefix, tag, text,
	prompt, input, suggestion,
	bg, timestamp, reset
)

inline Theme load_theme(const std::filesystem::path& path) {
	std::ifstream file(path);

	if(!file) {
		throw std::runtime_error("failed to open theme file: " + path.string());
	}

	const auto json = nlohmann::json::parse(file);
	auto storage = std::make_unique<ThemeStorage>(json.get<ThemeStorage>());
	return Theme(std::move(storage));
}

constexpr std::string_view severity_style(const Theme::StyleBody& style, Severity severity) {
	using enum Severity;

	switch(severity) {
		case trace:
			return style.trace;
		case debug:
			return style.debug;
		case info:
			return style.info;
		case warn:
			return style.warn;
		case error:
			return style.error;
		case fatal:
			return style.fatal;
		case console:
			return style.con;
		case console_error:
			return style.conerr;
		default:
			return {};
	}
}

} // posix, log, ember