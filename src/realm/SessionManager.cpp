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
	InternalID assigned_id = 0;

	{
		std::lock_guard guard(sessions_lock_);
		auto id = generate_id();
		sessions_.emplace(id, std::move(client));
		assigned_id = id;
	}

	ptr->on_close([&, assigned_id]() {
		stop(assigned_id);
	});

	ptr->start();
}

void SessionManager::stop(const InternalID session_id) {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(session_id); it != sessions_.end()) {
		id_recycle_.emplace(it->first);
		sessions_.erase(it);
	}
}

void SessionManager::stop_all() {
	std::lock_guard guard(sessions_lock_);

	while(!sessions_.empty()) {
		auto [_, client] = sessions_.pull(sessions_.begin());
		client->stop();
	}
}

auto SessionManager::generate_id() -> InternalID {
	if(id_recycle_.empty()) {
		return next_id_++;
	} else {
		auto id = id_recycle_.top();
		id_recycle_.pop();
		return id;
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

std::optional<ClientIdent> SessionManager::client_ident(const InternalID session_id) const {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(session_id); it != sessions_.end()) {
		return it->second->handler().uuid();
	}

	return std::nullopt;
}

SessionManager::~SessionManager() {
	stop_all();
}

} // realm, ember