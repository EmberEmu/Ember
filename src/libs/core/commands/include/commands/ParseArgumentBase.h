/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <any>
#include <string_view>

namespace ember::commands {

class ParseArgumentBase {
public:
	virtual std::any to_any(const std::string& token) = 0;

	virtual ~ParseArgumentBase() = default;
};

} // commands, ember