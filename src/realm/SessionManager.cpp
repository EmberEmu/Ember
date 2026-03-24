/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"
#include "ConnectionStats.h"

namespace ember::realm {

void SessionManager::stop(Client* session) {
	std::lock_guard guard(sessions_lock_);

	auto it = sessions_.find(session);
	
	if(it == sessions_.end()) {
		return;
	}

	auto client = sessions_.pull(it);
	//ClientConnection::async_shutdown(std::move(client));
}

void SessionManager::stop_all() {
	std::lock_guard guard(sessions_lock_);

	while(!sessions_.empty()) {
		auto client = sessions_.pull(sessions_.begin());
		//ClientConnection::async_shutdown(std::move(client));
	}
}

std::size_t SessionManager::count() const {
	return sessions_.size();
}

ConnectionStats SessionManager::aggregate_stats() const {
	std::lock_guard guard(sessions_lock_);

	ConnectionStats ag_stats {};

	for(const auto& session : sessions_) {
		const auto& stats = session->connection().stats();
		ag_stats.bytes_in += stats.bytes_in;
		ag_stats.bytes_out += stats.bytes_out;
		ag_stats.latency += stats.latency;
		ag_stats.messages_in += stats.messages_in;
		ag_stats.messages_out += stats.messages_out;
		ag_stats.async_receives += stats.async_receives;
		ag_stats.async_sends += stats.async_sends;
	}

	ag_stats.latency /= count(); // average latency
	return ag_stats;
}

SessionManager::~SessionManager() {
	stop_all();
}

} // realm, ember