/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace ember::blaze::library {

enum class Result {
	success,
	open_failed,
	close_failed,
	symbol_not_found,
	module_handle_failed,
	unknown
};

inline std::string_view result_to_string(const Result result) {
	using enum Result;

	switch(result) {
		case success:
			return "success";
		case open_failed:
			return "open failed";
		case close_failed:
			return "close failed";
		case symbol_not_found:
			return "symbol not found";
		case unknown:
			return "unknown error";
		case module_handle_failed:
			return "unable to retrieve module handle";
		default:
			return "unhandled result";
	}
}

struct Handle__ { int unused; }; typedef struct Handle__ *Handle;

namespace impl {

std::expected<void*, Result> find_symbol_base(Handle handle, const std::string& name);

}

template<typename _fn>
std::expected<_fn, Result> find_symbol(Handle handle, const std::string& name) {
	auto result = impl::find_symbol_base(handle, name);

	if(result) {
		return reinterpret_cast<_fn>(*result);
	} else {
		return std::unexpected(result.error());
	}
}

std::expected<Handle, Result> open(const std::string& name);
std::expected<Handle, Result> current_module();
Result close(Handle handle);

} // library, blaze, ember