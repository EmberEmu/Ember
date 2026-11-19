/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ParseArgument.h>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>

namespace ember::commands {

struct required_t{};
struct optional_t{};

inline constexpr required_t required = required_t();
inline constexpr optional_t optional = optional_t();

struct Argument {
	std::string name;
	bool required;
	std::shared_ptr<ParseArgumentBase> parser;         // todo, need to refactor some other bits
	std::reference_wrapper<const std::type_info> type; // this is okay, type_info lives for the duration of the program

	Argument(std::string name,
	         bool required,
	         std::unique_ptr<ParseArgumentBase> parser,
	         const std::type_info& type_info) noexcept
		: name(std::move(name))
		, required(required)
		, parser(std::move(parser))
		, type(type_info) {}
};

} // commands, ember