/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <commands/TypeMap.h>
#include <commands/UserData.h>
#include <string>

namespace ember::commands::detail {

const boost::unordered_flat_map<args::Type, std::type_index> types {
	{ args::Type::at_string,    typeid(std::string),    },
	{ args::Type::at_uint8,     typeid(std::uint8_t),   },
	{ args::Type::at_uint16,    typeid(std::uint16_t),  },
	{ args::Type::at_uint32,    typeid(std::uint32_t),  },
	{ args::Type::at_uint64,    typeid(std::uint64_t),  },
	{ args::Type::at_int8,      typeid(std::int8_t),    },
	{ args::Type::at_int16,     typeid(std::int16_t),   },
	{ args::Type::at_int32,     typeid(std::int32_t),   },
	{ args::Type::at_int64,     typeid(std::int64_t),   },
	{ args::Type::at_float,     typeid(float),          },
	{ args::Type::at_double,    typeid(double),         },
	{ args::Type::at_char,      typeid(char),           },
	{ args::Type::at_user_data, typeid(args::UserData), },
};

} // commands, ember 

