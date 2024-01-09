/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <span>
#include <string>
#include <cstdint>

namespace ember::stun {

class Transport {
public:
	virtual void connect() = 0;
	virtual void send(std::span<std::uint8_t> message) = 0;
	virtual ~Transport() = default;
};

} // stun, ember