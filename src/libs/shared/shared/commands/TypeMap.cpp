/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "TypeMap.h"
#include <string>

namespace ember::commands::detail {

const boost::unordered_flat_map<ArgumentType, std::type_index> types {
	{ ArgumentType::at_string, typeid(std::string),   },
	{ ArgumentType::at_uint8,  typeid(std::uint8_t),  },
	{ ArgumentType::at_uint16, typeid(std::uint16_t), },
	{ ArgumentType::at_uint32, typeid(std::uint32_t), },
	{ ArgumentType::at_uint64, typeid(std::uint64_t), },
	{ ArgumentType::at_int8,   typeid(std::int8_t),	  },
	{ ArgumentType::at_int16,  typeid(std::int16_t),  },
	{ ArgumentType::at_int32,  typeid(std::int32_t),  },
	{ ArgumentType::at_int64,  typeid(std::int64_t),  },
	{ ArgumentType::at_float,  typeid(float),         },
	{ ArgumentType::at_double, typeid(double),        },
	{ ArgumentType::at_char,   typeid(char),          },
};

} // commands, ember 

