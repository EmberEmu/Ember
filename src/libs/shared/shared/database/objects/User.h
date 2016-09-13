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
#include <cstdint>

namespace ember {

enum class PINMethod {
	NONE, FIXED, TOTP
};

class User {
	std::uint32_t id_;
	std::string user_;
	std::string v_;
	std::string s_;
	PINMethod pin_method_;
	std::uint64_t pin_;
	std::string totp_token_;
	bool banned_;
	bool suspended_;

public:
	User(std::uint32_t id, std::string username, std::string salt, std::string verifier,
	     PINMethod pin_method, std::uint64_t pin, std::string totp_token, bool banned, bool suspended)
         : id_(id), user_(std::move(username)), s_(std::move(salt)), v_(std::move(verifier)),
	       pin_(pin), pin_method_(pin_method), totp_token_(std::move(totp_token)) {
		banned_ = banned;
		suspended_ = suspended;
	}

	std::uint32_t id() const {
		return id_;
	}

	PINMethod pin_method() const {
		return pin_method_;
	}

	std::string totp_token() const{
		return totp_token_;
	}

	std::uint64_t pin() const {
		return pin_;
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