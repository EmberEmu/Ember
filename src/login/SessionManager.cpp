/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"
#include "LoginSession.h"

namespace ember {

void SessionManager::start(const std::shared_ptr<LoginNetSession>& session) {
	std::lock_guard<std::mutex> guard(sessions_lock_);
	sessions_.insert(session);
	session->start();
}

void SessionManager::stop(const std::shared_ptr<LoginNetSession>& session) {
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

} // ember