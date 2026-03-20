/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ArgumentMap.h>
#include <commands/AnyArg.h>

namespace ember::commands {

// boost program options does this by extending std::map, which is nice
// since it provides an easy interface but it's also naughty, so
// we'll make do with wrapping any functionality as we need it
class Arguments {
	ArgMap map_;

public:
	Arguments(ArgMap map)
		: map_(std::move(map)) {}

	const AnyArg& operator[](const std::string_view key) const {
		return map_.at(key);
	}

	bool contains(const std::string_view key) const {
		return map_.contains(key);
	}

	bool empty() const {
		return map_.empty();
	}
};

} // commands, ember