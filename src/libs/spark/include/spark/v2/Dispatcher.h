/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <span>
#include <cstdint>

namespace ember::spark::v2 {

class Dispatcher {
public:
	virtual void receive(std::span<const std::uint8_t> data) = 0;
	virtual ~Dispatcher() = default;
};

} // spark, ember