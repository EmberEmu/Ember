/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/uuid/uuid.hpp>
#include <string>

namespace ember { namespace spark {

enum class LinkState {
	LINK_UP, LINK_DOWN
};

struct Link {
	boost::uuids::uuid uuid;
	std::string description;
};

inline bool operator==(const Link& lhs, const Link& rhs) {
	return lhs.description == rhs.description && rhs.uuid == lhs.uuid;
}

}} // spark, ember