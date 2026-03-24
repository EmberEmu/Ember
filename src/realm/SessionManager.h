/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"
#include "Client.h"
#include <spark/buffers/allocators/TLSBlockAllocator.h>
#include <boost/unordered/unordered_flat_set.hpp>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember::realm {

struct ConnectionStats;

class SessionManager final {
	using ClientPtr = std::unique_ptr<Client, std::function<void(Client*)>>;

	constexpr inline static auto allocator_tag = "session_manager";

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
	boost::unordered_flat_set<ClientPtr, Hasher, KeyEqual> sessions_;
	mutable std::mutex sessions_lock_;

public:
	SessionManager() = default;
	~SessionManager();

	void emplace(ClientPtr client) {
		client->start();
		std::lock_guard guard(sessions_lock_);
		sessions_.emplace(std::move(client));
	}

	void stop(Client* session);
	void stop_all();
	std::size_t count() const;
	ConnectionStats aggregate_stats() const;
};

} // realm, ember