/*
 * Copyright (c) 2014 Ember
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

namespace ember { namespace connection_pool {

namespace sc = std::chrono;

template<typename ConType>
struct ConnDetail {
	ConnDetail(const ConType& connection) : conn(connection) {}
	ConType conn;
	bool dirty = false;
	bool checked_out = false;
	bool error = false;
	bool sweep = false;
	sc::seconds idle = sc::seconds(0);
};

template<typename ConType>
class Connection {
	bool released_ = false;
	std::mutex close_protect_;
	mutable std::reference_wrapper<ConnDetail<ConType>> detail_;
	typedef std::function<void(Connection<ConType>&)> FreeHandler;
	FreeHandler fh_;
	Connection(FreeHandler handler, ConnDetail<ConType>& detail)
	           : fh_(std::move(handler)), detail_(detail) {}

public:
	~Connection() {
		if(!released_) {
			fh_(*this);
		}
	}

	Connection(Connection<ConType>&& src)
	           : detail_(std::move(src.detail_)), fh_(std::move(src.fh_)) {
		src.released_ = true;
	}

	Connection<ConType>& operator=(Connection<ConType>&& src) {
		if(!released_) {
			fh_(*this);
		}

		released_ = src.released_;
		detail_ = src.detail_;
		return *this;
	}

	void close() {
		std::lock_guard<std::mutex> guard(close_protect_);

		if(!released_) {
			released_ = true;
			fh_(*this);
		}
	}

	Connection(const Connection<ConType>& src) = delete;
	Connection<ConType>& operator=(const Connection<ConType>& src) = delete;

	ConType operator()() { return detail_.get().conn; }
	ConType operator->() { return detail_.get().conn; }
	ConType operator*() { return detail_.get().conn; }

	template<typename A, typename B, typename C> friend class Pool;
};


}} //connection_pool, ember