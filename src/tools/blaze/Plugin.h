/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Common.h"
#include "Library.h"
#include <string>
#include <string_view>

namespace ember::blaze {

class Plugin final {
	library::Handle handle_;
	std::string name_;
	PluginID pid_;

public:
	Plugin(library::Handle handle, std::string name, PluginID pid)
		: handle_(handle)
		, name_(name)
		, pid_(pid) {}

	~Plugin() {
		library::close(handle_);
	}

	std::string_view name() const {
		return name_;
	}

	PluginID pid() const {
		return pid_;
	}

	Plugin(Plugin&& rhs) noexcept
		: handle_(rhs.handle_)
		, name_(rhs.name_)
		, pid_(rhs.pid_) {
		rhs.name_ = "";
		rhs.handle_ = nullptr;
		rhs.pid_ = 0;
	}

	Plugin& operator=(Plugin&& rhs) noexcept {
		handle_ = rhs.handle_;
		name_ = rhs.name_;
		rhs.name_ = "";
		rhs.handle_ = nullptr;
		rhs.pid_ = 0;
		return *this;
	}

	Plugin operator=(Plugin&) = delete;
	Plugin(Plugin&) = delete;
};

} // blaze, ember