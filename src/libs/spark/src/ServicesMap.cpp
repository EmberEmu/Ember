/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/ServicesMap.h>

namespace ember::spark {

std::vector<Link> ServicesMap::peer_services(messaging::Service service, Mode type) const {
	std::lock_guard<std::mutex> guard(lock_);

	auto& map = type == Mode::CLIENT? peer_clients_ : peer_servers_;
	auto it = map.find(service);

	if(it != map.end()) {
		return std::vector<Link>(it->second.begin(), it->second.end());
	}
	
	return std::vector<Link>();
}

void ServicesMap::register_peer_service(const Link& link, messaging::Service service, Mode type) {
	std::lock_guard<std::mutex> guard(lock_);

	switch(type) {
		case Mode::CLIENT:
			peer_clients_[service].emplace_front(link);
			break;
		case Mode::SERVER:
			peer_servers_[service].emplace_front(link);
			break;
	}
}

void ServicesMap::remove_peer(const Link& link) {
	std::lock_guard<std::mutex> guard(lock_);

	for(auto& list : peer_servers_) {
		list.second.erase_after(std::remove_if(list.second.begin(), list.second.end(), [&](const auto& arg) {
			return link == arg;
		}), list.second.end());
	}

	for(auto& list : peer_clients_) {
		list.second.erase_after(std::remove_if(list.second.begin(), list.second.end(), [&](const auto& arg) {
			return link == arg;
		}), list.second.end());
	}
}

} // spark, ember