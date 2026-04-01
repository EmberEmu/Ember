/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"
#include <utility>

namespace ember::realm {

void SessionManager::start(ClientPtr client) {
	auto ptr = client.get();
	SessionID assigned_id = 0;

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

bool SessionManager::stop(const SessionID session_id) {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(session_id); it != sessions_.end()) {
		sessions_.erase(it);
		return true;
	}

	return false;
}

void SessionManager::stop_all() {
	std::lock_guard guard(sessions_lock_);

	while(!sessions_.empty()) {
		auto [_, client] = sessions_.pull(sessions_.begin());
		client->stop();
	}
}

/*
 * Generates a new session ID, attempting to keep it below the defined wrap value.
 * IDs larger than the wrap threshold can be generated, but it'd be very impressive
 * if that happens in reality.

 * This is primarily about generating an ID that isn't going to be too onerous to
 * type in a command, while providing some protection against an ID being quickly
 * recycled between an ID being displayed and a command input acting on that ID
 * (use UUID for full protection against that).
 */
auto SessionManager::generate_id() -> SessionID {
	if(next_id_ > session_id_wrap) {
		next_id_ = 0;
	}

	while(sessions_.contains(next_id_)) {
		++next_id_;
	}

	return next_id_++;
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

std::optional<ClientIdent> SessionManager::client_ident(const SessionID id) const {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(id); it != sessions_.end()) {
		return it->second->handler().uuid();
	}

	return std::nullopt;
}

Client* SessionManager::client(const SessionID id) const {
	std::lock_guard guard(sessions_lock_);

	if(auto it = sessions_.find(id); it != sessions_.end()) {
		return it->second.get();
	}

	return nullptr;
}

SessionManager::~SessionManager() {
	stop_all();
}

} // realm, ember