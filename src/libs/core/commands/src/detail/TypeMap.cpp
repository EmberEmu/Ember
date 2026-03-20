/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <commands/detail/TypeMap.h>
#include <commands/UserData.h>
#include <string>

namespace ember::commands::detail {

const std::array<std::type_index, args::type_count> types {
	typeid(std::string),
	typeid(std::uint8_t),
	typeid(std::uint16_t),
	typeid(std::uint32_t),
	typeid(std::uint64_t),
	typeid(std::int8_t),
	typeid(std::int16_t),
	typeid(std::int32_t),
	typeid(std::int64_t),
	typeid(float),
	typeid(double),
	typeid(char),
	typeid(args::UserData),
};

} // commands, ember 

