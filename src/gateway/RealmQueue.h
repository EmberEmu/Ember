/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/ClientUUID.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <list>
#include <functional>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember {

class ClientConnection;

class RealmQueue final {
	typedef std::function<void()> LeaveQueueCB;
	typedef std::function<void(std::size_t)> UpdateQueueCB;

	struct QueueEntry {
		int priority;
		ClientUUID client;
		UpdateQueueCB on_update;
		LeaveQueueCB on_leave;

		bool operator>(const QueueEntry& rhs) const {
			return rhs.priority > priority;
		}

		bool operator<(const QueueEntry& rhs) const {
			return rhs.priority < priority;
		}
	};

	const std::chrono::milliseconds TIMER_FREQUENCY { 250 };

	boost::asio::steady_timer timer_;
	std::list<QueueEntry> queue_;
	std::mutex lock_;
	bool dirty_;

	void update_clients();
	void set_timer();

public:
	explicit RealmQueue(boost::asio::io_context& service) : timer_(service), dirty_(false) { }

	void enqueue(ClientUUID client, UpdateQueueCB on_update_cb,
	             LeaveQueueCB on_leave_cb, int priority = 0);
	void dequeue(const ClientUUID& client);
	void free_slot();
	void shutdown();
	std::size_t size() const;
};

} // ember