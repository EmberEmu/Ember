/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Link.h>
#include <boost/functional/hash.hpp>
#include <memory>
#include <unordered_map>
#include <shared_mutex>

namespace ember { namespace spark {

class NetworkSession;

class LinkMap {
	std::unordered_map<boost::uuids::uuid, std::weak_ptr<NetworkSession>,
	                   boost::hash<boost::uuids::uuid>> links_;
	mutable std::shared_timed_mutex lock_;

public:
	void register_link(const Link& link, std::shared_ptr<NetworkSession> net);
	void unregister_link(const Link& link);
	std::weak_ptr<NetworkSession> get(const Link& link) const;
};

}} // spark, ember