/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/ClientIdent.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember::realm {

class ClientConnection;
class EventDispatcher;

using namespace std::chrono_literals;

class RealmQueue final {
	struct QueueEntry {
		int priority;
		ClientIdent client;

		bool operator>(const QueueEntry& rhs) const {
			return rhs.priority > priority;
		}

		bool operator<(const QueueEntry& rhs) const {
			return rhs.priority < priority;
		}
	};

	static constexpr auto default_frequency { 250ms };
	const std::chrono::milliseconds frequency_;

	boost::asio::steady_timer timer_;
	std::list<QueueEntry> queue_;
	EventDispatcher& dispatcher_;
	std::mutex lock_;
	std::atomic_bool dirty_;

	bool empty();
	void update_clients();
	void set_timer();

public:
	constexpr static std::size_t npos = 0;

	explicit RealmQueue(boost::asio::io_context& service,
	                    EventDispatcher& dispatcher,
	                    std::chrono::milliseconds frequency = default_frequency)
		: frequency_(frequency)
		, timer_(service)
		, dispatcher_(dispatcher)
		, dirty_(false) { }

	~RealmQueue();

	void enqueue(ClientIdent client, int priority = 0);
	void dequeue(const ClientIdent& client);
	std::size_t poll(const ClientIdent& client);
	void free_slot();
	void shutdown();
	std::size_t size() const;
};

} // realm, ember