/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Link.h>
#include <spark/v2/Common.h>
#include <string>
#include <span>
#include <cstdint>

namespace ember::spark::v2 {

class Handler {
public:
	virtual std::string type() = 0;
	virtual std::string name() = 0;
	virtual void on_message(const spark::v2::Link& link, std::span<const std::uint8_t> msg) = 0;
	virtual void on_link_up(const spark::v2::Link& link) = 0;
	virtual void on_link_down(const spark::v2::Link& link) = 0;

	virtual ~Handler() = default;
};

} // spark, ember