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

namespace ember { namespace connection_pool {

template<typename ConType>
struct ConnDetail {
	ConnDetail(const ConType& connection) : conn(connection) {}
	ConType conn;
	bool dirty = false;
	bool checked_out = false;
	bool error = false;
	unsigned int idle = 0;
};

template<typename ConType>
class Connection {
	bool released_ = false;
	mutable ConnDetail<ConType>& detail_;
	typedef std::function<void(Connection<ConType>&)> FreeHandler;
	const FreeHandler fh_;
	Connection(FreeHandler handler, ConnDetail<ConType>& detail)
		: fh_(std::move(handler)), detail_(detail) {}

public:
	~Connection() {
		if (!released_) {
			fh_(*this);
		}
	}

	Connection(Connection<ConType>&& src)
		       : detail_(std::move(src.detail_)), fh_(std::move(src.fh_)) {
		src.released_ = true;
	}

	Connection<ConType>& operator=(Connection<ConType>&& src) {
		detail(src.detail_);
		fh(std::move(src.fh_));
		src.released_ = true;
		return *this;
	}

	Connection(const Connection<ConType>& src) = delete;
	Connection<ConType>& operator=(const Connection<ConType>& src) = delete;

	ConType operator()() { return detail_.conn; }
	ConType operator->() { return detail_.conn; }

	template<typename A, typename B, typename C> friend class Pool;
};


}} //connection_pool, ember