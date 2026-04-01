/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace ember::realm {

struct ConnectionStats {
	std::size_t bytes_in;
	std::size_t bytes_out;
	std::size_t messages_in;
	std::size_t messages_out;
	std::size_t async_receives;
	std::size_t async_sends;
	std::uint32_t latency;
};

} // realm, ember