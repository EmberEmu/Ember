/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/UTF8String.h>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

namespace ember {

enum class PINMethod {
	NONE, FIXED, TOTP
};

class User {
	std::uint32_t id_;
	utf8_string user_;
	std::string v_;
	std::vector<std::uint8_t> s_;
	PINMethod pin_method_;
	std::uint64_t pin_;
	std::string totp_token_;
	bool banned_;
	bool suspended_;
	bool survey_request_;
	bool subscriber_;

public:
	User(std::uint32_t id, utf8_string username, std::vector<std::uint8_t> salt, std::string verifier,
	     PINMethod pin_method, std::uint64_t pin, std::string totp_token, bool banned, bool suspended,
	     bool survey_request, bool subscriber)
         : id_(id), user_(std::move(username)), s_(std::move(salt)), v_(std::move(verifier)),
	       pin_(pin), pin_method_(pin_method), totp_token_(std::move(totp_token)),
	       survey_request_(survey_request), subscriber_(subscriber) {
		banned_ = banned;
		suspended_ = suspended;
	}

	std::uint32_t id() const {
		return id_;
	}

	PINMethod pin_method() const {
		return pin_method_;
	}

	const std::string& totp_token() const {
		return totp_token_;
	}

	std::uint64_t pin() const {
		return pin_;
	}

	const std::string& verifier() const {
		return v_;
	}

	const std::vector<std::uint8_t>& salt() const {
		return s_;
	}

	utf8_string username() const {
		return user_;
	}

	bool banned() const {
		return banned_;
	}

	bool suspended() const {
		return suspended_;
	}

	bool survey_request() const {
		return survey_request_;
	}

	bool subscriber() const {
		return subscriber_;
	}
};

} //ember