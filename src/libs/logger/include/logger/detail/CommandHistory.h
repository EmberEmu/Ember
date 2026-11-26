/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <cstddef>

namespace ember::log::detail {

class CommandHistory final {
	const std::size_t max_entries_;

	std::deque<std::string> history_;
	std::size_t index_;
	std::string stashed_;

public:
	static constexpr auto default_max_entries { 10u };

	explicit CommandHistory(std::size_t max_entries = default_max_entries);

	enum class ScrollDirection {
		up, down
	};

	std::optional<std::string_view> scroll(ScrollDirection direction, std::string_view current = {});
	void insert(std::string command);

	bool empty() const;
	std::size_t size() const;
	constexpr std::size_t capacity() const;
	std::string_view current() const;
	bool scrolling() const;
};

} // detail, log, ember