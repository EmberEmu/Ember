/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "EventTypes.h"
#include <concepts>

namespace ember::realm {

struct Event {
	EventType type;
	
	Event() = delete;
	~Event() = default;

	template<std::derived_from<Event> _ty>
	auto& as() {
		return static_cast<_ty&>(*this);
	}

	template<std::derived_from<Event> _ty>
	auto& as() const {
		return static_cast<const _ty&>(*this);
	}

protected:
	Event(EventType type) : type(type) {}
};

} // realm, ember