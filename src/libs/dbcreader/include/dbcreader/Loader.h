/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <initializer_list>
#include <span>
#include <string>
#include <vector>

namespace ember::dbc {

struct Storage;

class Loader {
public:
	virtual Storage load() const = 0;
	virtual Storage load(std::initializer_list<const std::string> whitelist) const = 0;
	virtual Storage load(std::span<const std::string> whitelist) const = 0;
	virtual ~Loader() = default;
};

} // dbc, ember