/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Peers.h>
#include <spark/v2/RemotePeer.h>

namespace ember::spark::v2 {

void Peers::add(std::string key, std::shared_ptr<RemotePeer> peer) {
	std::lock_guard<std::mutex> guard(lock_);
	peers_.emplace(std::move(key), std::move(peer));
}

void Peers::remove(const std::string& key) {
	std::lock_guard<std::mutex> guard(lock_);
	peers_.erase(key);
}

std::shared_ptr<RemotePeer> Peers::find(const std::string& key) {
	std::lock_guard<std::mutex> guard(lock_);

	if(auto it = peers_.find(key); it != peers_.end()) {
		return it->second;
	}

	return nullptr;
}

} // spark, ember