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
#include <boost/unordered/unordered_flat_set.hpp>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember::realm {

class SessionManager final {
	struct Hasher {
		using is_transparent = void;

		std::size_t operator()(Client* p) const {
			return std::hash<Client*>{}(p);
		}

		std::size_t operator()(const ClientPtr& p) const {
			return std::hash<const Client*>{}(p.get()); 
		}
	};

	struct KeyEqual {
		using is_transparent = void;

		template<typename _lhs, typename _rhs>
		auto operator()(const _lhs& lhs, const _rhs& rhs) const {
			return to_ptr(lhs) == to_ptr(rhs);
		}

	private:
		static const Client* to_ptr(const Client* p) {
			return p; 
		}

		static const Client* to_ptr(const ClientPtr& p) {
			return p.get(); 
		}
	};

	using SessionsMap = boost::unordered_flat_set<ClientPtr, Hasher, KeyEqual>;

	SessionsMap sessions_;
	mutable std::mutex sessions_lock_;

public:
	using locked_iterator = SessionIterator<SessionsMap::iterator>;
	using locked_const_iterator = SessionIterator<SessionsMap::const_iterator>;

	SessionManager() = default;
	~SessionManager();

	void start(ClientPtr client);
	void stop(Client* session);
	void stop_all();
	std::size_t count() const;

	locked_const_iterator begin() const;
	locked_const_iterator end() const;
};

} // realm, ember