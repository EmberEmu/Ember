/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::ports {

// Exists only so NAT-PMP/PCP & UPnP can use the same definition
enum class Protocol {
	ALL, // PCP only
	TCP,
	UDP
};

// Nowhere else to put it
constexpr struct use_future_t{} use_future;
constexpr struct use_awaitable_t{} use_awaitable;

} // ports, ember