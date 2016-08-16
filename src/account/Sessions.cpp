/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Sessions.h"

namespace ember {

bool Sessions::register_session(std::uint32_t account_id, Botan::BigInt key) {
	std::lock_guard<std::mutex> guard(lock_);

	auto it = sessions_.find(account_id);

	if(!allow_overwrite_ && it != sessions_.end()) {
		return false;
	}

	sessions_[account_id] = key;
	return true;
}

boost::optional<Botan::BigInt> Sessions::lookup_session(std::uint32_t account_id) {
	std::lock_guard<std::mutex> guard(lock_);
	auto it = sessions_.find(account_id);

	if(it == sessions_.end()) {
		return boost::optional<Botan::BigInt>();
	}

	return boost::optional<Botan::BigInt>(it->second);
}

} // ember