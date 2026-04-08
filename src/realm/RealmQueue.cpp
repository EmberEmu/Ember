/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmQueue.h"
#include "Events.h"
#include "EventDispatcher.h"

namespace ember::realm {

void RealmQueue::set_timer() {
	timer_.expires_after(frequency_);
	timer_.async_wait([this](const boost::system::error_code& ec) {
		if(!ec) { // if ec is set, the timer was aborted (shutdown)
			if(dirty_) {
				update_clients();
			}

			if(!empty()) {
				set_timer();
			}
		}
	});
}

bool RealmQueue::empty() {
	std::lock_guard guard(lock_);
	return queue_.empty();
}

/*
 * Periodically update clients with their current queue position
 * This is done with a timer rather than as players leave the queue/server
 * in order to reduce network traffic with longer queues where queue positions
 * are changing rapidly
 */
void RealmQueue::update_clients() {
	std::lock_guard guard(lock_);

	std::size_t position = 1;

	for(auto& entry : queue_) {
		dispatcher_.post(entry.client, QueuePosition(position));
		++position;
	}

	dirty_ = false;
}

void RealmQueue::enqueue(ClientIdent client, int priority) {
	std::lock_guard guard(lock_);

	if(queue_.empty()) {
		set_timer();
	}

	queue_.emplace_back(priority, client);

	// guaranteed to be a stable sort - not the most efficient way to have queue priority
	// but allows for multiple priority levels without multiple hard-coded queues
	queue_.sort();
	dirty_ = true;
}

/* 
 * Signals that a currently queued player has decided to disconnect rather
 * hang around in the queue
 */
void RealmQueue::dequeue(const ClientIdent& client) {
	std::lock_guard guard(lock_);

	if(auto it = std::ranges::find(queue_, client, &QueueEntry::client); it != queue_.end()) {
		queue_.erase(it);
	}

	dirty_ = true;
}

/* 
 * Signals that a player occupying a server slot has disconnected, thus
 * allowing the player at the front of the queue to connect
 */
void RealmQueue::free_slot() {
	std::lock_guard guard(lock_);

	if(queue_.empty()) {
		return;
	}

	auto& entry = queue_.front();
	const Event event { EventType::queue_success };
	dispatcher_.post(entry.client, event);
	queue_.pop_front();
	dirty_ = true;
}

std::size_t RealmQueue::poll(const ClientIdent& client) {
	std::lock_guard guard(lock_);

	std::size_t position = 1;

	for(auto& entry : queue_) {
		if(entry.client == client) {
			return position;
		}

		++position;
	}

	return npos;
}
void RealmQueue::shutdown() {
	std::lock_guard guard(lock_);
	timer_.cancel();
}

std::size_t RealmQueue::size() const {
	return queue_.size();
}

RealmQueue::~RealmQueue() {
	shutdown();
}

} // realm, ember