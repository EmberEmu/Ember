/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmQueue.h"
#include "ClientConnection.h"
#include <game_protocol/server/SMSG_AUTH_RESPONSE.h>

namespace ember {

void RealmQueue::set_timer() {
	timer_.expires_from_now(TIMER_FREQUENCY);
	timer_.async_wait([this](const boost::system::error_code& ec) {
		if(!ec) { // if ec is set, the timer was aborted (shutdown)
			update_clients();
		}
	});
}

/*
 * Periodically update clients with their current queue position
 * This is done with a timer rather than as players leave the queue/server
 * in order to reduce network traffic with longer queues where queue positions
 * are changing rapidly
 */
void RealmQueue::update_clients() {
	std::lock_guard<std::mutex> guard(lock_);
	std::size_t position = 1;

	for(auto& entry : queue_) {
		send_position(position, entry.client);
		++position;
	}

	set_timer();
}

void RealmQueue::send_position(std::size_t position, std::shared_ptr<ClientConnection> client) {
	client->socket().get_io_service().dispatch([client, position]() {
		protocol::SMSG_AUTH_RESPONSE packet;
		packet.result = protocol::ResultCode::AUTH_WAIT_QUEUE;
		packet.queue_position = position;
		client->send(packet);
	});
}

void RealmQueue::enqueue(std::shared_ptr<ClientConnection> client, LeaveQueueCB callback, int priority) {
	std::lock_guard<std::mutex> guard(lock_);

	if(queue_.empty()) {
		set_timer();
	}

	queue_.emplace_back(QueueEntry{priority, client, callback});

	// guaranteed to be a stable sort - not the most efficient way to have queue priority
	// but allows for multiple priority levels without multiple hard-coded queues
	queue_.sort();
}

/* Signals that a currently queued player has decided to disconnect rather
 * hang around in the queue */
void RealmQueue::dequeue(const std::shared_ptr<ClientConnection>& client) {
	std::lock_guard<std::mutex> guard(lock_);

	for(auto i = queue_.begin(); i != queue_.end(); ++i) {
		if(i->client == client) {
			queue_.erase(i);
			break;
		}
	}

	if(queue_.empty()) {
		timer_.cancel();
	}
}

/* Signals that a player occupying a server slot has disconnected, thus
 * allowing the player at the front of the queue to connect */
void RealmQueue::free_slot() {
	std::lock_guard<std::mutex> guard(lock_);

	if(queue_.empty()) {
		return;
	}

	auto& entry = queue_.front();
	entry.callback();
	queue_.pop_front();

	if(queue_.empty()) {
		timer_.cancel();
	}
}

void RealmQueue::shutdown() {
	std::lock_guard<std::mutex> guard(lock_);
	timer_.cancel();
}

std::size_t RealmQueue::size() const {
	return queue_.size();
}

} // ember