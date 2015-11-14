/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/SessionManager.h>
#include <spark/NetworkSession.h>

namespace ember { namespace spark {

void SessionManager::start(std::shared_ptr<NetworkSession> session) {
	std::lock_guard<std::mutex> guard(sessions_lock_);
	sessions_.insert(session);
	session->start();
}

void SessionManager::stop(std::shared_ptr<NetworkSession> session) {
	std::lock_guard<std::mutex> guard(sessions_lock_);
	sessions_.erase(session);
	session->stop();
}

void SessionManager::stop_all() {
	for(auto& session : sessions_) {
		session->stop();
	}

	std::lock_guard<std::mutex> guard(sessions_lock_);
	sessions_.clear();
}

std::size_t SessionManager::count() const {
	return sessions_.size();
}


}} // spark, ember