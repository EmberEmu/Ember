/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>

namespace ConnectionPool {

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
	bool released = false;
	mutable ConnDetail<ConType>& detail;
	typedef std::function<void(Connection<ConType>&)> FreeHandler;
	const FreeHandler fh;
	Connection(FreeHandler handler, ConnDetail<ConType>& detail)
		: fh(std::move(handler)), detail(detail) {}

public:
	~Connection() {
		if (!released) {
			fh(*this);
		}
	}

	Connection(Connection<ConType>&& src)
		       : detail(src.detail), fh(std::move(src.fh)) {
		src.released = true;
	}

	Connection<ConType>& operator=(Connection<ConType>&& src) {
		detail(src.detail);
		fh(std::move(src.fh));
		src.released = true;
		return *this;
	}

	Connection(const Connection<ConType>& src) = delete;
	Connection<ConType>& operator=(const Connection<ConType>& src) = delete;

	ConType operator()() { return connection; }
	ConType operator->() { return connection; }

	template<typename A, typename B, typename C> friend class Pool;
};


} //ConnectionPool