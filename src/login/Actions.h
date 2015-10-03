/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/objects/User.h>
#include <shared/database/daos/UserDAO.h>
#include <boost/optional.hpp>
#include <string>
#include <utility>

namespace ember {

class Action {
protected:
	Action() = default;

public:
	virtual void execute() = 0;
	virtual ~Action() = default;
};

class FetchSessionKeyAction : public Action {
	const std::string username_;
	const dal::UserDAO& user_src_;
	boost::optional<std::string> key_;
	std::exception_ptr exception_;

public:
	FetchSessionKeyAction(std::string username, const dal::UserDAO& user_src)
	                      : username_(std::move(username)), user_src_(user_src) {}

	virtual void execute() final override try {
		key_ = user_src_.session_key(username_);
	} catch(std::exception) {
		exception_ = std::current_exception();
	}

	boost::optional<std::string> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(key_);
	}
};

class StoreSessionAction : public Action {
	User user_;
	const std::string ip_, key_;
	const dal::UserDAO& user_src_;
	std::exception_ptr exception_;

public:
	StoreSessionAction(User user, std::string ip, std::string key, const dal::UserDAO& user_src)
	                   : user_(std::move(user)), ip_(std::move(ip)), key_(std::move(key)),
	                     user_src_(user_src){}

	virtual void execute() final override try {
		user_src_.record_last_login(user_, ip_);
		user_src_.session_key(user_.username(), key_); // todo - user vs username, why?
	} catch(std::exception) {
		exception_ = std::current_exception();
	}

	boost::optional<User> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(user_);
	}
};

class FetchUserAction : public Action {
	const std::string username_;
	const dal::UserDAO& user_src_;
	boost::optional<User> user_;
	std::exception_ptr exception_;

public:
	FetchUserAction(std::string username, const dal::UserDAO& user_src)
	                : username_(std::move(username)), user_src_(user_src) {}

	virtual void execute() final override try {
		user_ = user_src_.user(username_);
	} catch(std::exception) {
		exception_ = std::current_exception();
	}

	boost::optional<User> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(user_);
	}
};

} // ember