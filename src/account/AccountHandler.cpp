/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountHandler.h"
#include <shared/threading/ThreadPool.h>

namespace ember {

AccountHandler::AccountHandler(dal::UserDAO& user_dao, ThreadPool& pool)
	: user_dao_(user_dao),
	  pool_(pool) {}

std::optional<std::uint32_t> AccountHandler::lookup_id(const std::string& username) {
	const auto user = user_dao_.user(username);
	
	if(!user) {
		return std::nullopt;
	}

	return user->id();
}

void AccountHandler::lookup_id(const std::string& username, LookupCB cb) {
	pool_.run([=, this]() {
		try {
			cb(lookup_id(username));
		} catch(std::exception&) {
			cb(std::unexpected(false));
		}
	});
}

} // ember