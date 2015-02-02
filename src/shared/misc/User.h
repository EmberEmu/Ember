/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>

namespace ember {

class User {
	std::string user_;
	std::string v_;
	std::string s_;

public:
	User(std::string username, std::string salt, std::string verifier)
	    : user_(username), s_(std::move(salt)), v_(std::move(verifier)) { }

	std::string verifier() {
		return v_;
	}

	std::string salt() {
		return s_;
	}

	std::string username() {
		return user_;
	}
};

} //ember