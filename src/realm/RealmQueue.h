/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ConfigStore.h"
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
	mutable std::mutex lock_;
	std::atomic_bool dirty_;
	std::size_t active_;
	const ConfigStore& cstore_;

	bool empty();
	void update_clients();
	void set_timer();

public:
	constexpr static std::size_t npos = 0;
	
	enum class Result {
		success,
		queued,
	};

	explicit RealmQueue(boost::asio::io_context& service,
	                    EventDispatcher& dispatcher,
	                    ConfigStore& config_store,
	                    std::chrono::milliseconds frequency = default_frequency)
		: frequency_(frequency)
		, timer_(service)
		, dispatcher_(dispatcher)
		, dirty_(false)
		, active_(0)
		, cstore_(config_store) { }

	~RealmQueue();

	Result enqueue(const ClientIdent& client, int priority = 0);
	void dequeue(const ClientIdent& client);
	std::size_t poll(const ClientIdent& client);
	void free_slot();
	void shutdown();
	std::size_t size() const;
	std::size_t occupied() const;
};

} // realm, ember