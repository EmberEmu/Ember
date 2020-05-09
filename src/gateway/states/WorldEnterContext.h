/*
 * Copyright (c) 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember::world_enter {

enum class State {
	TEMPORARY
};

struct Context {
	State state { State::TEMPORARY };
	std::uint16_t character_guid {};
};

} // world_enter, ember