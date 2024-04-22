/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Message.h> // todo, move
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::spark::v2 {

class Channel;

struct Link {
	std::string peer_banner;
	std::string service_name;
	std::weak_ptr<Channel> net;
};

inline bool operator==(const Link& lhs, const Link& rhs) {
	return lhs.service_name == rhs.service_name;
}

inline bool operator!=(const Link& lhs, const Link& rhs) {
	return !(lhs == rhs);
}

typedef std::function<void(const Link&, std::expected<bool, Message>)> TrackedHandler; // todo move

} // spark, ember