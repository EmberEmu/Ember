/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Client.h"
#include "SessionIterator.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <cstddef>
#include <cstdint>

namespace ember::realm {

class SessionManager final {
public:
	using SessionID = std::uint32_t;

private:
	constexpr static auto session_id_wrap = 100'000;

	using SessionsMap = boost::unordered_flat_map<SessionID, ClientPtr>;

	SessionsMap sessions_;
	SessionID next_id_ = 0;

	mutable std::mutex sessions_lock_;

	SessionID generate_id();

	bool stop(SessionID id);
	void stop_all();

public:
	using locked_iterator = SessionIterator<SessionsMap::iterator>;
	using locked_const_iterator = SessionIterator<SessionsMap::const_iterator>;

	SessionManager() = default;
	~SessionManager();

	void start(ClientPtr client);

	std::size_t count() const;
	std::optional<ClientIdent> client_ident(SessionID id) const;

	locked_const_iterator begin() const;
	locked_const_iterator end() const;
};

} // realm, ember