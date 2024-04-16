/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace ember::spark::v2 {

class RemotePeer;

struct Link {
	std::string banner;
	std::string service;
	std::weak_ptr<RemotePeer> net;
	std::uint8_t channel_id;
};

inline bool operator==(const Link& lhs, const Link& rhs) {
	return rhs.channel_id == lhs.channel_id
		&& rhs.net.owner_before(lhs.net) == lhs.net.owner_before(rhs.net);
}

inline bool operator!=(const Link& lhs, const Link& rhs) {
	return !(lhs == rhs);
}

} // spark, ember