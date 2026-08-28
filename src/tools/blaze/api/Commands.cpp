/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Commands.h"
#include "../InterfaceContainer.h"

namespace ember::blaze {

Command command_create(const SizedString* name, const SizedString* description) {
	auto registry = InterfaceContainer::get_instance().command_root();
	auto command = registry->create({ name->data, name->size });
	command->description({ description->data, description->size });
	InterfaceContainer::get_instance().plugin_command_registry()->insert("test", command);

	return {
		.impl = command.get()
	};
}

template<typename T>
bool command_arg_register(commands::Command& command, const std::string& name, std::uint8_t type) {
	switch(type) {
		case cat_string:
			command.argument<std::string>(name, T{});
			break;
		case cat_float:
			command.argument<float>(name, T{});
			break;
		case cat_double:
			command.argument<double>(name, T{});
			break;
		case cat_int_8:
			command.argument<std::int8_t>(name, T{});
			break;
		case cat_int_16:
			command.argument<std::int16_t>(name, T{});
			break;
		case cat_int_32:
			command.argument<std::int32_t>(name, T{});
			break;
		case cat_int_64:
			command.argument<std::int64_t>(name, T{});
			break;
		case cat_uint_8:
			command.argument<std::uint8_t>(name, T{});
			break;
		case cat_uint_16:
			command.argument<std::uint16_t>(name, T{});
			break;
		case cat_uint_32:
			command.argument<std::uint32_t>(name, T{});
			break;
		case cat_uint_64:
			command.argument<std::uint64_t>(name, T{});
			break;
		default:
			return false;
	}

	return true;
}

bool command_add_argument(Command command, const SizedString* name, std::uint8_t type, bool required) {
	auto pcr = InterfaceContainer::get_instance().plugin_command_registry();
	auto result = pcr->lookup(command.impl);

	if(!result) {
		return false;
	}

	std::string view(name->data, name->size);

	if(required) {
		return command_arg_register<commands::required_t>(*result, std::move(view), type);
	} else {
		return command_arg_register<commands::optional_t>(*result, std::move(view), type);
	}
}

bool command_destroy(Command command) {
	auto pcr = InterfaceContainer::get_instance().plugin_command_registry();
	auto result = pcr->lookup(command.impl);

	if(!result) {
		return false;
	}

	auto registry = InterfaceContainer::get_instance().command_root();
	return registry->erase(result);
}

bool command_callback(Command command) {
	auto pcr = InterfaceContainer::get_instance().plugin_command_registry();
	auto result = pcr->lookup(command.impl);

	if(!result) {
		return false;
	}

	// todo, this is going to take a whole lot of work to get working over a C boundary
	return true;
}

} // blaze, ember