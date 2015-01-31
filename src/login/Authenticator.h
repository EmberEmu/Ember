/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Server.h>
#include <memory>
#include <vector>

namespace ember {

namespace dal { class UserDAO; }

struct GameVersion;

class Authenticator {
	dal::UserDAO& users_;
	const std::vector<GameVersion>& versions_;
	std::unique_ptr<srp6::Server> auth_;

public:
	Authenticator(dal::UserDAO& users, const std::vector<GameVersion>& versions)
	              : versions_(versions), users_(users) { }

	//VS2013 bug workaround, again
	Authenticator(Authenticator&& rhs) : versions_(rhs.versions_), users_(rhs.users_) {
		auth_ = std::move(rhs.auth_);
	}

	void verify_client_version(const GameVersion& version);
};

} //ember