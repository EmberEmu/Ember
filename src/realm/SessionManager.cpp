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
#include <shared/utility/polyfill/inplace_vector>
#include <shared/utility/UncheckedBackInserter.h>
#include <utility>
#include <cassert>

namespace ember::realm {

SessionManager::SessionManager(boost::asio::io_context& ioc, std::size_t buckets, log::Logger& logger)
	: timer_(ioc)
	, logger_(logger)
	, token_(queue_)
	, sessions_(buckets) {
	start_timer();
}

void SessionManager::start_timer() {
	timer_.expires_after(config::session_collect_frequency);
	timer_.async_wait([&](auto ec) {
		if(ec) {
			return;
		}

		process_queue();
		collect();
		start_timer();
	});
}

std::size_t SessionManager::bulk_insert(std::span<unique_client_ptr> clients) {
	auto count = clients.size();
	std::lock_guard guard(sessions_lock_);

	for(auto& client : clients) {
		if(!client->stopped()) { // let stopped clients destruct
			sessions_.emplace(reserve_slot_id(), std::move(client));
		} else {
			--count;
		}
	}

	peak_count_ = std::max(sessions_.size(), peak_count_);
	return count;
}

void SessionManager::process_queue() {
	std::size_t count = 0;
	std::inplace_vector<unique_client_ptr, max_bulk_dequeue> dequeued;

	while(true) {
#if __has_include(<inplace_vector>) // todo, temporary until supported compiler versions are bumped
		queue_.try_dequeue_bulk(token_, unchecked_back_inserter(dequeued), max_bulk_dequeue);
#else
		queue_.try_dequeue_bulk(token_, std::back_inserter(dequeued), max_bulk_dequeue);
#endif

		if(dequeued.empty()) {
			break;
		}

		count += bulk_insert(dequeued);
		dequeued.clear();
	}

	if(count) {
		LOG_TRACE(logger_, "Inserted {} queued session{}", count, count > 1? "s" : "");
	}
}

void SessionManager::enqueue(unique_client_ptr client) {
	queue_.enqueue(std::move(client));
}

void SessionManager::stop() {
	std::lock_guard guard(sessions_lock_);
	timer_.cancel();
	sessions_.clear();
	slots_.reset();
}

/*
 * Reserves a new session ID, attempting to keep it below the defined wrap value.
 * IDs larger than the wrap threshold can be generated, but it'd be very impressive
 * if that happens in reality.

 * This is primarily about generating an ID that isn't going to be too onerous to
 * type in a command, while providing some protection against an ID being quickly
 * recycled between an ID being displayed and a command input acting on that ID
 * (use UUID for full protection against that).
 */
auto SessionManager::reserve_slot_id() -> SessionID {
	auto id = next_id_;

	// iterate over all slots to try to claim a free ID
	for(std::size_t scanned = 0; scanned < slots_.size(); ++scanned) {
		if(!slots_.test(id)) {
			slots_.set(id);

			auto next = id + 1;

			if(next == slots_.size()) {
				next = 0;
			}

			next_id_ = next;
			return id;
		}

		// wraparound
		if(++id == slots_.size()) {
			id = 0;
		}
	}

	// if all slots are taken, go outside of the bitset's range
	// until we find an unused ID - should never really happen
	// but we can handle it if the server is *that* busy
	id = slots_.size();

	while(sessions_.contains(id)) {
		++id;
	}

	return id;
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
	std::size_t collected = 0;
	
	{ 
		std::lock_guard guard(sessions_lock_);

		for(auto it = sessions_.begin(); it != sessions_.end();) {
			if(it->second->stopped()) {
				release_slot_id(it->first);
				it = sessions_.erase(it);
				++collected;
			} else {
				++it;
			}
		}
	}

	if(collected) {
		LOG_TRACE(logger_, "Collected {} session{}", collected, collected > 1? "s": "");
	}
}

void SessionManager::release_slot_id(const SessionID id) {
	if(id < slots_.size()) {
		assert(slots_.test(id));
		slots_.reset(id);
	}
}

} // realm, ember