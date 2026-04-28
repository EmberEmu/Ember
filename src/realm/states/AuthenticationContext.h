/*
 * Copyright (c) 2020 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/client/AuthSession.h>
#include <cstdint>

namespace ember::realm::authentication {

enum class State {
	not_authed,
	in_progress,
	in_queue,
	success,
	failed
};

struct Context {
	State state;
	std::uint32_t seed;
	std::uint32_t account_id;
	protocol::cmsg_auth_session packet;
};

} // authentication, realm, ember