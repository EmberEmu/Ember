/*
 * Copyright (c) 2014, 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <chrono>
#include <mutex>
#include <utility>

namespace ember::connection_pool {

namespace sc = std::chrono;
using namespace std::chrono_literals;

const int CACHELINE_SIZE = 64; // todo, use preprocessor?

template<typename ConType>
struct alignas(CACHELINE_SIZE) ConnDetail {
	ConType conn;
	unsigned int id = 0;
	bool empty_slot = true;
	bool dirty = false;
	bool checked_out = false;
	bool error = false;
	bool sweep = false;
	bool refresh = false;
	sc::seconds idle = 0s;

	ConnDetail(const ConType& connection, unsigned int id) : conn(connection), id(id), empty_slot(false) {}
	ConnDetail() = default;

	void reset() {
		empty_slot = true;
		dirty = checked_out = error = sweep = refresh = false;
	}
};

template<typename ConType>
class Connection {
	typedef std::function<void(Connection<ConType>&)> ReleaseHandler;

	ReleaseHandler rh_;
	bool released_ = false;
	std::mutex close_protect_;
	mutable std::reference_wrapper<ConnDetail<ConType>> detail_;

	Connection(ReleaseHandler handler, ConnDetail<ConType>& detail)
	           : rh_(std::move(handler)), detail_(detail) {}

public:
	~Connection() {
		if(!released_) {
			rh_(*this);
		}
	}

	Connection(Connection<ConType>&& src)
	           : detail_(std::move(src.detail_)), rh_(std::move(src.rh_)) {
		src.released_ = true;
	}

	Connection<ConType>& operator=(Connection<ConType>&& src) {
		if(!released_) {
			rh_(*this);
		}

		released_ = src.released_;
		detail_ = src.detail_;
		return *this;
	}

	void release() {
		std::lock_guard<std::mutex> guard(close_protect_);

		if(!released_) {
			released_ = true;
			rh_(*this);
		}
	}

	Connection(const Connection<ConType>& src) = delete;
	Connection<ConType>& operator=(const Connection<ConType>& src) = delete;

	ConType operator->() { return detail_.get().conn; }
	ConType operator*() { return detail_.get().conn; }

	template<typename A, typename B, typename C> friend class Pool;
};


} // connection_pool, ember