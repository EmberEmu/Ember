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
	timer_.async_wait(std::bind(&RealmQueue::update_clients, this, std::placeholders::_1));
}

void RealmQueue::update_clients(const boost::system::error_code& ec) {
	if(ec) { // if ec is set, the timer was aborted (shutdown)
		return;
	}

	std::lock_guard<std::mutex> guard(lock_);
	std::size_t position = 1;

	for(auto& entry : queue_) {
		send_position(position, entry.client);
		++position;
	}

	set_timer();
}

void RealmQueue::send_position(std::size_t position, std::shared_ptr<ClientConnection> client) {
	auto packet = std::make_shared<protocol::SMSG_AUTH_RESPONSE>();
	packet->result = protocol::ResultCode::AUTH_WAIT_QUEUE;
	packet->queue_position = position;
	client->send(protocol::ServerOpcodes::SMSG_AUTH_RESPONSE, packet);
}

void RealmQueue::enqueue(std::shared_ptr<ClientConnection> client, LeaveQueueCB callback) {
	std::lock_guard<std::mutex> guard(lock_);

	if(queue_.empty()) {
		set_timer();
	}

	queue_.emplace_back(QueueEntry{client, callback});
}

void RealmQueue::dequeue(std::shared_ptr<ClientConnection> client) {
	std::lock_guard<std::mutex> guard(lock_);

	for(auto i = queue_.begin(); i != queue_.end(); ++i) {
		if(i->client == client) {
			queue_.erase(i);
		}
	}

	if(queue_.empty()) {
		timer_.cancel();
	}
}

void RealmQueue::decrement() {
	std::lock_guard<std::mutex> guard(lock_);

	if(queue_.empty()) {
		return;
	}

	auto entry = queue_.front();
	queue_.pop_front();

	entry.callback();

	if(queue_.empty()) {
		timer_.cancel();
	}
}

void RealmQueue::shutdown() {
	timer_.cancel();
}

std::size_t RealmQueue::size() const {
	return queue_.size();
}

} // ember