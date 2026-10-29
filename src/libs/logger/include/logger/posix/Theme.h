/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <cassert>

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
		rebind();
	}

	Theme(Theme&& other) noexcept
		: ThemeView(other)
		, storage(std::move(other.storage)) {
		other = Theme{};
	}

	Theme& operator=(Theme&& other) noexcept {
		if(this != &other) {
			ThemeView::operator=(other);
			storage = std::move(other.storage);
			other = Theme{};
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
			} else {
				storage.reset();
			}

			static_cast<ThemeView&>(*this) = static_cast<const ThemeView&>(other);
			rebind();
		}

		return *this;
	}
};

} // posix, log, ember