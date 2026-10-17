/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <string_view>

namespace ember::log::posix {

struct LogTheme {
	std::string_view name;
	std::string_view desc;

	struct StyleBody {
		std::string_view trace;
		std::string_view debug;
		std::string_view info;
		std::string_view warn;
		std::string_view error;
		std::string_view fatal;
		std::string_view con;
		std::string_view conerr;
	} tag, text;

	// input styling
	std::string_view prompt;
	std::string_view input;
	std::string_view suggestion;

	// misc styling
	std::string_view bg;
	std::string_view timestamp;
	std::string_view reset;
};

enum class StyleSegment {
	tag, text
};

constexpr std::string_view severity_colour(const LogTheme::StyleBody& style, Severity severity) {
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

constexpr std::string_view theme_colour(const LogTheme& theme, Severity severity, StyleSegment segment) {
	using enum StyleSegment;

	switch(segment) {
		case tag:
			return severity_colour(theme.tag, severity);
		case text:
			return severity_colour(theme.text, severity);
	}

	std::unreachable();
}

} // posix, log, ember