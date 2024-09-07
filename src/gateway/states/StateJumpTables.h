/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include <protocol/Opcodes.h>
#include <array>
#include <utility>

namespace ember { 

struct ClientContext;
struct Event;

template<typename T>
using JumpTable = std::array<T, STATES_NUM>;

/*
 * Wrapper that ensures any resizing of the jump tables without also
 * having added the new state pointers will prevent compilation rather
 * than allowing the tables to contain junk entries
 * 
 * Compilers can see optimise this away, so it has no runtime penalty
 */
template<typename T>
struct no_default {
	T t;

	template<typename ...Args>
	constexpr auto operator()(Args&& ...args) const {
		t(std::forward<Args>(args)...);
	}

	constexpr no_default(T t) : t(t) {};
	no_default() = delete;
};


using state_func = no_default<void(*)(ClientContext&)>;
using event_handler = no_default<void(*)(ClientContext&, const Event*)>;
using packet_handler = no_default<void(*)(ClientContext&, protocol::ClientOpcode)>;

extern const JumpTable<state_func> enter_states;
extern const JumpTable<state_func> exit_states;
extern const JumpTable<packet_handler> update_packet;
extern const JumpTable<event_handler> update_event;

} // ember