/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../Library.h"
#include <Windows.h>

namespace ember::blaze::library {

std::expected<Handle, Result> open(const cstring_view name) {
	auto module = LoadLibrary(name.c_str());

	if(module) {
		return reinterpret_cast<Handle>(module);
	} else {
		return std::unexpected(Result::open_failed);
	}
}

std::expected<void*, Result> find_symbol_base(Handle handle, const cstring_view name) {
	auto module = reinterpret_cast<HMODULE>(handle);
	auto ptr = GetProcAddress(module, name.c_str());

	if(ptr) {
		return ptr;
	} else {
		return std::unexpected(Result::symbol_not_found);
	}
}

Result close(Handle handle) {
	auto module = reinterpret_cast<HMODULE>(handle);
	return FreeLibrary(module)? Result::success : Result::close_failed;
}

} // library, blaze, ember