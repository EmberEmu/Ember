/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"
#include "GatewayClient.h"

namespace ember {

void SessionManager::start(std::shared_ptr<GatewayClient> session) {
	std::lock_guard<std::mutex> guard(sessions_lock_);

	auto handle = session.get();
	sessions_.insert(std::move(session));
	handle->start();
}

void SessionManager::stop(GatewayClient* session) {
	std::lock_guard<std::mutex> guard(sessions_lock_);

	// todo, change to unique_ptr extract in C++17
	auto it = std::find_if(sessions_.begin(), sessions_.end(), [session](auto& value) {
		return session == value.get();
	});

	GatewayClient::async_shutdown(*it);
	sessions_.erase(it);

}

void SessionManager::stop_all() {
	std::lock_guard<std::mutex> guard(sessions_lock_);
	
	for(auto& client : sessions_) {
		GatewayClient::async_shutdown(client);
	}

	sessions_.clear();
}

SessionManager::~SessionManager() {
	stop_all();
}

} // ember