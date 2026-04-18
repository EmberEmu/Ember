/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionManager.h"
#include "ConfigConsts.h"
#include <logger/Logger.h>
#include <utility>

namespace ember::realm {

SessionManager::SessionManager(boost::asio::io_context& ioc, thread::ServicePool& pool, log::Logger& logger)
	: timer_(ioc)
	, pool_(pool)
	, logger_(logger) {
	initiate_collection();
}

void SessionManager::initiate_collection() {
	timer_.expires_after(config::session_collect_frequency);
	timer_.async_wait([&](auto ec) {
		if(ec) {
			return;
		}

		collect();
		initiate_collection();
	});
}

void SessionManager::insert(unique_client_ptr client) {
	std::lock_guard guard(sessions_lock_);
	sessions_.emplace(generate_id(), std::move(client));
	peak_count_ = std::max(sessions_.size(), peak_count_);
}

void SessionManager::stop() {
	std::lock_guard guard(sessions_lock_);
	timer_.cancel();
	sessions_.clear();
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
	std::lock_guard guard(sessions_lock_);
	return sessions_.size();
}

std::size_t SessionManager::peak_count() const {
	std::lock_guard guard(sessions_lock_);
	return peak_count_;
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
		return it->second->uuid();
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

void SessionManager::collect() {
	std::lock_guard guard(sessions_lock_);
	std::size_t collected = 0;

	for(auto it = sessions_.begin(); it != sessions_.end();) {
		if(it->second->stopped()) {
			it = sessions_.erase(it);
			++collected;
		} else {
			++it;
		}
	}

	if(collected) {
		LOG_TRACE(logger_, "Collected {} sessions", collected);
	}
}

} // realm, ember