/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authenticator.h"
#include <srp6/Server.h>
#include <shared/database/daos/UserDAO.h>

namespace ember {

auto Authenticator::verify_client_version(const GameVersion& version) -> PATCH_STATE {
	if(std::find(versions_.begin(), versions_.end(), version) != versions_.end()) {
		return PATCH_STATE::OK;
	}

	//Figure out whether any of the allowed client versions are newer than the client.
	//If so, there's a chance that it can be patched.
	for(auto v : versions_) {
		if(v > version) {
			return PATCH_STATE::TOO_OLD;
		}
	}

	return PATCH_STATE::TOO_NEW;
}

auto Authenticator::check_account(const std::string& username) -> ACCOUNT_STATUS try {
	boost::optional<User> user = users_.user(username);

	if(!user) {
		return ACCOUNT_STATUS::NOT_FOUND;
	}

	srp6::Generator gen(srp6::Generator::GROUP::_256_BIT);
	auth_ = std::make_unique<srp6::Server>(gen, user->verifier());

	return ACCOUNT_STATUS::OK;
} catch(std::exception& e) {
	return ACCOUNT_STATUS::DAL_ERROR;
}

} //ember