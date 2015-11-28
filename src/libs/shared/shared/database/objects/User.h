/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <utility>

namespace ember {

class User {
	std::uint32_t id_;
	std::string user_;
	std::string v_;
	std::string s_;
	bool banned_;
	bool suspended_;

public:
	User(std::uint32_t id, std::string username, std::string salt, std::string verifier,
	     bool banned, bool suspended) : id_(id), user_(std::move(username)),
	     s_(std::move(salt)), v_(std::move(verifier)) {
		banned_ = banned;
		suspended_ = suspended;
	}

	std::uint32_t id() const {
		return id_;
	}

	std::string verifier() const {
		return v_;
	}

	std::string salt() const {
		return s_;
	}

	std::string username() const {
		return user_;
	}

	bool banned() const {
		return banned_;
	}

	bool suspended() const {
		return suspended_;
	}
};

} //ember