/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <nlohmann/json.hpp>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace ember::log::posix {

template<typename string_type>
class BasicLogTheme {
public:
	struct StyleBody {
		string_type trace;
		string_type debug;
		string_type info;
		string_type warn;
		string_type error;
		string_type fatal;
		string_type con;
		string_type conerr;
	};

private:
	template<typename other_string_type>
	static void bind_style(StyleBody& dst, const typename BasicLogTheme<other_string_type>::StyleBody& src) noexcept {
		dst.trace = src.trace;
		dst.debug = src.debug;
		dst.info = src.info;
		dst.warn = src.warn;
		dst.error = src.error;
		dst.fatal = src.fatal;
		dst.con = src.con;
		dst.conerr = src.conerr;
	}

public:
	string_type name;
	string_type desc;

	StyleBody prefix, tag, text;

	// input styling
	string_type prompt;
	string_type input;
	string_type suggestion;

	// misc styling
	string_type bg;
	string_type timestamp;
	string_type reset;

	template<typename other_string_type>
	requires std::same_as<string_type, std::string_view>
	void bind_from(const BasicLogTheme<other_string_type>& other) noexcept {
		name = other.name;
		desc = other.desc;

		bind_style<other_string_type>(prefix, other.prefix);
		bind_style<other_string_type>(tag, other.tag);
		bind_style<other_string_type>(text, other.text);

		prompt = other.prompt;
		input = other.input;
		suggestion = other.suggestion;

		bg = other.bg;
		timestamp = other.timestamp;
		reset = other.reset;
	}

	~BasicLogTheme() = default;
};

using ThemeView    = BasicLogTheme<std::string_view>;
using ThemeStorage = BasicLogTheme<std::string>;

class Theme final : public ThemeView {
	std::unique_ptr<ThemeStorage> storage;

	void rebind() noexcept {
		if(storage) {
			bind_from(*storage);
		}
	}

public:
	Theme() = default;

	explicit Theme(ThemeView theme)
		: ThemeView(theme) {}

	explicit Theme(std::unique_ptr<ThemeStorage> storage)
		: storage(std::move(storage)) {
		assert(storage);
		bind_from(*this->storage);
	}

	Theme(Theme&& other) noexcept
		: ThemeView(other)
		, storage(std::move(other.storage)) {}

	Theme& operator=(Theme&& other) noexcept {
		if(this != &other) {
			ThemeView::operator=(other);
			storage = std::move(other.storage);
		}

		return *this;
	}

	Theme(const Theme& other)
		: ThemeView(static_cast<const ThemeView&>(other)) {
		if(other.storage) {
			storage = std::make_unique<ThemeStorage>(*other.storage);
			rebind();
		}
	}

	Theme& operator=(const Theme& other) {
		if(this != &other) {
			if(other.storage) {
				storage = std::make_unique<ThemeStorage>(*other.storage);
			}

			static_cast<ThemeView&>(*this) = static_cast<const ThemeView&>(other);
			rebind();
		}

		return *this;
	}
};

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

enum class StyleSegment {
	prefix, tag, text
};

constexpr std::string_view severity_colour(const Theme::StyleBody& style, Severity severity) {
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

constexpr std::string_view theme_colour(const Theme& theme, Severity severity, StyleSegment segment) {
	using enum StyleSegment;

	switch(segment) {
		case prefix:
			return severity_colour(theme.prefix, severity);
		case tag:
			return severity_colour(theme.tag, severity);
		case text:
			return severity_colour(theme.text, severity);
	}

	std::unreachable();
}

} // posix, log, ember