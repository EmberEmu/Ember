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
#include <queue>
#include <cstddef>
#include <cstdint>

namespace ember::realm {

class SessionManager final {
public:
	using InternalID = std::uint32_t;

private:
	using SessionsMap = boost::unordered_flat_map<InternalID, ClientPtr>;

	SessionsMap sessions_;
	InternalID next_id_ = 0;
	std::priority_queue<InternalID> id_recycle_;

	mutable std::mutex sessions_lock_;

	InternalID generate_id();

public:
	using locked_iterator = SessionIterator<SessionsMap::iterator>;
	using locked_const_iterator = SessionIterator<SessionsMap::const_iterator>;

	SessionManager() = default;
	~SessionManager();

	void start(ClientPtr client);
	void stop(InternalID id);
	void stop_all();

	std::size_t count() const;
	std::optional<ClientIdent> client_ident(InternalID session_id) const;

	locked_const_iterator begin() const;
	locked_const_iterator end() const;
};

} // realm, ember