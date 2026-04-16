/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/unordered/unordered_flat_set.hpp>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember {

class LoginSession;
template<typename T> class NetworkSession;

using LoginNetSession = NetworkSession<LoginSession>;

class SessionManager final {
	boost::unordered_flat_set<std::shared_ptr<LoginNetSession>> sessions_;
	std::mutex sessions_lock_;

public:
	void start(std::shared_ptr<LoginNetSession>&& session);
	void stop(const std::shared_ptr<LoginNetSession>& session);
	void stop_all();
	std::size_t count() const;
};

} // ember