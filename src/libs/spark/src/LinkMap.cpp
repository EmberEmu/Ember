/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/LinkMap.h>
#include <spark/NetworkSession.h>

namespace ember { namespace spark {

void LinkMap::register_link(const Link& link, std::shared_ptr<NetworkSession> net) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	links_[link.uuid] = std::weak_ptr<NetworkSession>(net);
}

void LinkMap::unregister_link(const Link& link) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	links_.erase(link.uuid);
}

std::weak_ptr<NetworkSession> LinkMap::get(const Link& link) const {
	std::shared_lock<std::shared_timed_mutex> guard(lock_);
	return links_.at(link.uuid);
}

}} // spark, ember