/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Services_generated.h"
#include <spark/Link.h>
#include <forward_list>
#include <mutex>
#include <unordered_map>

namespace ember::spark::inline v1 {

class ServicesMap {
	std::unordered_map<messaging::Service, std::forward_list<Link>> peer_servers_;
	std::unordered_map<messaging::Service, std::forward_list<Link>> peer_clients_;
	mutable std::mutex lock_;

public:
	enum class Mode { CLIENT, SERVER };

	std::vector<Link> peer_services(messaging::Service service, Mode type) const;
	void register_peer_service(const Link& link, messaging::Service service, Mode type);
	void remove_peer(const Link& link);
};

} // spark, ember