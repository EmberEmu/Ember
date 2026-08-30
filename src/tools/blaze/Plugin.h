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
#include "PluginState.h"
#include <string>
#include <string_view>
#include <thread>

namespace ember::blaze {

class Plugin final {
	library::Handle handle_;
	std::string name_;
	std::string name_short_;
	PluginID pid_;
	PluginState state_ { PluginState::loaded }; // todo, temp
	std::thread worker_;

	void reset() {
		name_ = "";
		name_short_ = "";
		handle_ = nullptr;
		pid_ = 0;
	}

public:
	Plugin(library::Handle handle, std::string name, std::string name_short, PluginID pid)
		: handle_(handle)
		, name_(name)
		, name_short_(name_short)
		, pid_(pid) {}

	~Plugin() {
		// todo, doesn't need to be explicit but need to sort other stuff first
		if(worker_.joinable()) {
			worker_.join();
		}

		library::close(handle_);
	}

	std::string_view name() const {
		return name_;
	}

	std::string_view name_short() const {
		return name_short_;
	}

	PluginID pid() const {
		return pid_;
	}

	Plugin(Plugin&& rhs) noexcept
		: handle_(rhs.handle_)
		, name_(rhs.name_)
		, name_short_(rhs.name_short_)
		, pid_(rhs.pid_) {
		rhs.reset();
	}

	Plugin& operator=(Plugin&& rhs) noexcept {
		handle_ = rhs.handle_;
		name_ = rhs.name_;
		name_short_ = rhs.name_short_;
		pid_ = rhs.pid_;
		rhs.reset();
		return *this;
	}

	Plugin operator=(Plugin&) = delete;
	Plugin(Plugin&) = delete;
};

} // blaze, ember