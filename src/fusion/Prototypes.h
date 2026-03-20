/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/cstring_view.hpp>
#include <array>
#include <string_view>

namespace ember::fusion {

struct LibConf {
	cstring_view name;
	cstring_view libname;
	cstring_view create_fn;
	cstring_view destroy_fn;
};

enum ServiceIndex {
	service_account,
	service_character,
	service_login,
	service_realm,
	service_world,
	service_mdns,
	service_invalid = -1
};

static constexpr auto max_service_index = ServiceIndex::service_mdns;

inline ServiceIndex string_to_idx(const std::string_view service) {
	if(service == "account") {
		return ServiceIndex::service_account;
	} else if(service == "mdns") {
		return ServiceIndex::service_mdns;
	} else if(service == "login") {
		return ServiceIndex::service_login;
	} else if(service == "realm") {
		return ServiceIndex::service_realm;
	} else if(service == "world") {
		return ServiceIndex::service_world;
	} else if(service == "character") {
		return ServiceIndex::service_character;
	} else {
		return ServiceIndex::service_invalid;
	}
}

#define LIB_PREFIX "lib"

#if defined(_WIN32)
#define LIB_EXT(name) LIB_PREFIX name ".dll"
#elif defined(__linux__)
#define LIB_EXT(name) LIB_PREFIX name ".so"
#elif defined(__APPLE__) && defined(__MACH__)
#define LIB_EXT(name) LIB_PREFIX name ".dylib"
#endif

inline constexpr std::array<LibConf, max_service_index + 1> lib_props {{
	{ "account",   LIB_EXT("account"),   "create_account",   "destroy_account"   },
	{ "character", LIB_EXT("character"), "create_character", "destroy_character" },
	{ "login",     LIB_EXT("login"),     "create_login",     "destroy_login"     },
	{ "realm",     LIB_EXT("realm"),     "create_realm",     "destroy_realm"     },
	{ "world",     LIB_EXT("world"),     "create_world",     "destroy_world"     },
	{ "mdns",      LIB_EXT("mdns"),      "create_mdns",      "destroy_mdns"      }
}};

} // fusion, ember

