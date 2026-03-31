/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"

namespace ember::realm {

void SessionManager::start(ClientPtr client) {
	auto ptr = client.get();

	client->on_close([&, ptr]() {
		stop(ptr);
	});

	{
		std::lock_guard guard(sessions_lock_);
		sessions_.emplace(std::move(client));
	}

	ptr->start();
}

void SessionManager::stop(Client* session) {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(session); it != sessions_.end()) {
		sessions_.erase(it);
	}
}

void SessionManager::stop_all() {
	std::lock_guard guard(sessions_lock_);

	while(!sessions_.empty()) {
		auto client = sessions_.pull(sessions_.begin());
		client->stop();
	}
}

std::size_t SessionManager::count() const {
	return sessions_.size();
}

auto SessionManager::begin() const -> locked_const_iterator {
	return SessionIterator(sessions_.begin(), sessions_lock_);
}

auto SessionManager::end() const -> locked_const_iterator {
	return SessionIterator(sessions_.end());
}

SessionManager::~SessionManager() {
	stop_all();
}

} // realm, ember