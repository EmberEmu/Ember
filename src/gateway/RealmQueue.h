/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <chrono>
#include <list>
#include <functional>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember {

class ClientConnection;

class RealmQueue {
	typedef std::function<void()> LeaveQueueCB;

	struct QueueEntry {
		int priority;
		std::shared_ptr<ClientConnection> client;
		LeaveQueueCB callback;

		bool operator>(const QueueEntry& rhs) const {
			return rhs.priority > priority;
		}

		bool operator<(const QueueEntry& rhs) const {
			return rhs.priority < priority;
		}
	};

	const std::chrono::milliseconds TIMER_FREQUENCY { 250 };

	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
	std::list<QueueEntry> queue_;
	std::mutex lock_;

	void send_position(std::size_t position, std::shared_ptr<ClientConnection> client);
	void update_clients();
	void set_timer();

public:
	RealmQueue::RealmQueue(boost::asio::io_service& service) : timer_(service) { }

	void enqueue(std::shared_ptr<ClientConnection> client, LeaveQueueCB callback, int priority = 0);
	void dequeue(std::shared_ptr<ClientConnection> client);
	void decrement();
	void shutdown();
	std::size_t size() const;
};

} // ember