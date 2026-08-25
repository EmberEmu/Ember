/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Library.h"
#include <string_view>

namespace ember::blaze {

class Plugin final {
	library::Handle handle_;
	std::string_view name_;

public:
	Plugin(library::Handle handle, std::string_view name)
		: handle_(handle)
		, name_(name) {}

	~Plugin() {
		library::close(handle_);
	}

	Plugin(Plugin&& rhs) noexcept
		: handle_(rhs.handle_)
		, name_(rhs.name_) {
		rhs.name_ = "";
		rhs.handle_ = nullptr;
	}

	Plugin& operator=(Plugin&& rhs) noexcept {
		handle_ = rhs.handle_;
		name_ = rhs.name_;
		rhs.name_ = "";
		rhs.handle_ = nullptr;
		return *this;
	}

	Plugin operator=(Plugin&) = delete;
	Plugin(Plugin&) = delete;
};

} // blaze, ember