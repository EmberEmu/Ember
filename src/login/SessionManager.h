/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <mutex>
#include <unordered_set>
#include <cstddef>

namespace ember {

class LoginSession;
template<typename T> class NetworkSession;

using LoginNetSession = NetworkSession<LoginSession>;

class SessionManager final {
	std::unordered_set<std::shared_ptr<LoginNetSession>> sessions_;
	std::mutex sessions_lock_;

public:
	void start(const std::shared_ptr<LoginNetSession>& session);
	void stop(const std::shared_ptr<LoginNetSession>& session);
	void stop_all();
	std::size_t count() const;
};

} // ember