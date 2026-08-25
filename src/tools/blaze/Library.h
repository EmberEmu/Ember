/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/cstring_view.hpp>
#include <expected>

namespace ember::blaze::library {

enum class Result {
	success,
	open_failed,
	close_failed,
	symbol_not_found,
	unknown
};

struct Handle__ { int unused; }; typedef struct Handle__ *Handle;

std::expected<void*, Result> find_symbol_base(Handle handle, const cstring_view name);

template<typename _fn>
std::expected<_fn, Result> find_symbol(Handle handle, const cstring_view name) {
	auto result = find_symbol_base(handle, name);

	if(result) {
		return reinterpret_cast<_fn>(*result);
	} else {
		return std::unexpected(result.error());
	}
}

std::expected<Handle, Result> open(const cstring_view name);
Result close(Handle handle);

} // library, blaze, ember