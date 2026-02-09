/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"
#include <spark/buffers/allocators/TLSBlockAllocator.h>
#include <boost/unordered/unordered_flat_set.hpp>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember::gateway {

struct ConnectionStats;

#ifndef PREALLOCATED_SESSIONS_PER_THREAD
	#define PREALLOCATED_SESSIONS_PER_THREAD 32
#endif

class SessionManager final {
	using SessionAllocator = spark::io::TLSBlockAllocator<
		ClientConnection,
		PREALLOCATED_SESSIONS_PER_THREAD,
		spark::io::NoRefCounting,
		spark::io::SafeEntrant
	>;

	using ClientConnPtr = std::unique_ptr<ClientConnection, std::function<void(ClientConnection*)>>;

	struct Hasher {
		using is_transparent = void;

		std::size_t operator()(ClientConnection* p) const {
			return std::hash<ClientConnection*>{}(p);
		}

		std::size_t operator()(const ClientConnPtr& p) const {
			return std::hash<const ClientConnection*>{}(p.get()); 
		}
	};

	struct KeyEqual {
		using is_transparent = void;

		template<typename _lhs, typename _rhs>
		auto operator()(const _lhs& lhs, const _rhs& rhs) const {
			return to_ptr(lhs) == to_ptr(rhs);
		}

	private:
		static const ClientConnection* to_ptr(const ClientConnection* p) {
			return p; 
		}

		static const ClientConnection* to_ptr(const ClientConnPtr& p) {
			return p.get(); 
		}
	};

	SessionAllocator allocator_;
	boost::unordered_flat_set<ClientConnPtr, Hasher, KeyEqual> sessions_;
	mutable std::mutex sessions_lock_;

public:
	~SessionManager();

	template<typename... Args>
	void emplace(Args&&... args) {
		auto client = ClientConnPtr(
			allocator_.allocate(std::forward<Args>(args)...), [&](auto ptr) {
				allocator_.deallocate(ptr);
			}
		);

		std::lock_guard guard(sessions_lock_);
		sessions_.emplace(std::move(client));
	}

	void stop(ClientConnection* session);
	void stop_all();
	std::size_t count() const;
	ConnectionStats aggregate_stats() const;
};

} // gateway, ember