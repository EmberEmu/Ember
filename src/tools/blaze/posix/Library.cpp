/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../Library.h"
#include <dlfcn.h>

namespace ember::blaze::library {

namespace impl {

std::expected<void*, Result> find_symbol_base(Handle handle, const cstring_view name) {
	auto lib = reinterpret_cast<void*>(handle);
	auto sym = dlsym(lib, name.c_str());

	if(sym) {
		return sym;
	} else {
		return std::unexpected(Result::symbol_not_found);
	}
}

} // impl

std::expected<Handle, Result> open(const cstring_view name) {
	auto lib = dlopen(name.c_str(), 0);

	if(lib) {
		return reinterpret_cast<Handle>(lib);
	} else {
		return std::unexpected(Result::open_failed);
	}
}


Result close(Handle handle) {
	auto fn_ptr = reinterpret_cast<void*>(handle);
	return dlclose(fn_ptr) == 0? Result::success : Result::close_failed;
}

} // library, blaze, ember