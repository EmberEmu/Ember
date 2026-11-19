/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ParseArgument.h>
#include <memory>
#include <string>

namespace ember::commands {

template<typename _ty>
std::shared_ptr<Command> Command::argument(std::string argument, required_t) {
	auto parser = std::make_unique<ParseArgument<_ty>>();
	return insert_argument(std::move(argument), std::move(parser), typeid(_ty));
}

template<typename _ty>
std::shared_ptr<Command> Command::argument(std::string argument, optional_t) {
	auto parser = std::make_unique<ParseArgument<_ty>>();
	return insert_optional_argument(std::move(argument), std::move(parser), typeid(_ty));
}

} // commands, ember