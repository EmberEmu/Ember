/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Commands.h"
#include "State.h"
#include "../ServiceContextImpl.h"

namespace ember::blaze {

Command command_create(const CountedString* name, const CountedString* description) {
	auto registry = ctx->get()->plugin_commands.get();
	assert(registry);

	auto command = commands::create(std::string(name->data, name->size));
	command->description(std::string(description->data, description->size));
	registry->insert("test", command);

	return {
		.impl = command.get()
	};
}

template<typename T>
bool command_arg_register(commands::Command& command, std::string name, std::uint8_t type) {
	switch(type) {
		case cat_string:
			command.argument<std::string>(std::move(name), T{});
			break;
		case cat_float:
			command.argument<float>(std::move(name), T{});
			break;
		case cat_double:
			command.argument<double>(std::move(name), T{});
			break;
		case cat_int_8:
			command.argument<std::int8_t>(std::move(name), T{});
			break;
		case cat_int_16:
			command.argument<std::int16_t>(std::move(name), T{});
			break;
		case cat_int_32:
			command.argument<std::int32_t>(std::move(name), T{});
			break;
		case cat_int_64:
			command.argument<std::int64_t>(std::move(name), T{});
			break;
		case cat_uint_8:
			command.argument<std::uint8_t>(std::move(name), T{});
			break;
		case cat_uint_16:
			command.argument<std::uint16_t>(std::move(name), T{});
			break;
		case cat_uint_32:
			command.argument<std::uint32_t>(std::move(name), T{});
			break;
		case cat_uint_64:
			command.argument<std::uint64_t>(std::move(name), T{});
			break;
		default:
			return false;
	}

	return true;
}

bool command_add_argument(Command command, const CountedString* name, std::uint8_t type, bool required) {
	auto registry = ctx->get()->plugin_commands.get();
	assert(registry);

	auto result = registry->lookup(command.impl);

	if(!result) {
		return false;
	}

	std::string name_str(name->data, name->size);

	if(required) {
		return command_arg_register<commands::required_t>(*result, std::move(name_str), type);
	} else {
		return command_arg_register<commands::optional_t>(*result, std::move(name_str), type);
	}
}

bool command_destroy(Command command) {
	auto registry = ctx->get()->plugin_commands.get();
	assert(registry);

	auto result = registry->lookup(command.impl);

	if(!result) {
		return false;
	}

	return true; // temp, like everything in this file
}

bool command_callback(Command command) {
	auto registry = ctx->get()->plugin_commands.get();
	assert(registry);

	auto result = registry->lookup(command.impl);

	if(!result) {
		return false;
	}

	// todo, this is going to take a whole lot of work to get working over a C boundary
	return true;
}

} // blaze, ember