/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "unique_client_ptr.h"
#include "SessionIterator.h"
#include <logger/LoggerFwd.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <moodycamel/concurrentqueue.h>
#include <bitset>
#include <mutex>
#include <optional>
#include <span>
#include <cstddef>
#include <cstdint>

namespace ember::realm {

class SessionManager final {
public:
	using SessionID = std::uint32_t;

private:
	constexpr static auto session_id_wrap  = 32'768;
	constexpr static auto max_bulk_dequeue = 1024;

	using SessionsMap = boost::unordered_flat_map<SessionID, unique_client_ptr>;

	SessionsMap sessions_;
	SessionID next_id_ = 0;
	std::size_t peak_count_ = 0;
	boost::asio::steady_timer timer_;
	log::Logger& logger_;
	moodycamel::ConcurrentQueue<unique_client_ptr> queue_;
	moodycamel::ConsumerToken token_;
	std::bitset<session_id_wrap> slots_;

	mutable std::mutex sessions_lock_;

	void collect();
	void start_timer();
	void process_queue();
	std::size_t bulk_insert(std::span<unique_client_ptr> clients);
	SessionID reserve_slot_id();
	void release_slot_id(SessionID id);

public:
	using locked_iterator = SessionIterator<SessionsMap::iterator>;
	using locked_const_iterator = SessionIterator<SessionsMap::const_iterator>;

	SessionManager(boost::asio::io_context& ioc, std::size_t buckets, log::Logger& logger);

	void enqueue(unique_client_ptr client);
	void stop();

	std::size_t count() const;
	std::size_t peak_count() const;
	std::optional<ClientIdent> client_ident(SessionID id) const;
	Client* client(const SessionID id) const;

	locked_const_iterator begin() const;
	locked_const_iterator end() const;
};

} // realm, ember