/*
 * Copyright (c) 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/client/AuthSession.h>
#include <cstdint>

namespace ember::authentication {

enum class State {
	NOT_AUTHED, IN_PROGRESS, IN_QUEUE, SUCCESS, FAILED
};

struct Context {
	State state { State::NOT_AUTHED };
	std::uint32_t seed {};
	std::vector<protocol::client::AuthSession::AddonData> addon_data;
};

} // authentication, ember