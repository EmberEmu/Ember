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
#include <mutex>
#include <cstddef>
#include <cstdint>

namespace ember::realm {

class SessionManager final {
public:
	using SessionID = std::uint32_t;

private:
	constexpr static auto session_id_wrap = 100'000;

	using SessionsMap = boost::unordered_flat_map<SessionID, unique_client_ptr>;

	SessionsMap sessions_;
	SessionID next_id_ = 0;
	std::size_t peak_count_ = 0;
	boost::asio::steady_timer timer_;
	log::Logger& logger_;

	mutable std::mutex sessions_lock_;

	void collect();
	void initiate_collection();

	SessionID generate_id();

public:
	using locked_iterator = SessionIterator<SessionsMap::iterator>;
	using locked_const_iterator = SessionIterator<SessionsMap::const_iterator>;

	SessionManager(boost::asio::io_context& ioc, log::Logger& logger);

	void insert(unique_client_ptr client);
	void stop();

	std::size_t count() const;
	std::size_t peak_count() const;
	std::optional<ClientIdent> client_ident(SessionID id) const;
	Client* client(const SessionID id) const;

	locked_const_iterator begin() const;
	locked_const_iterator end() const;
};

} // realm, ember