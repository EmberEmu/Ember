/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AccountService.h"
#include <shared/database/objects/User.h>
#include <shared/database/daos/UserDAO.h>
#include <boost/optional.hpp>
#include <exception>
#include <future>
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

class TestAction final : public Action {
	std::promise<int> result_;
	const AccountService& account_svc_;
	std::uint32_t account_id;
	std::exception_ptr exception_;
	int temp_;

	void callback(AccountService::Result result, boost::optional<Botan::BigInt> key) {
		result_.set_value(52);
	}

	std::future<int> locate() {
		account_svc_.locate_session(5, std::bind(&TestAction::callback, this,
			std::placeholders::_1, std::placeholders::_2));
		return result_.get_future();
	}

public:
	TestAction(const AccountService& account_svc, std::uint32_t acct_id)
	           : account_svc_(account_svc), account_id(account_id) { }

	virtual void execute() override try {
		std::future<int> foo = locate();
		temp_ = foo.get();
	} catch(std::exception) {
		exception_ = std::current_exception();
	}

	int get_result() {
		return temp_;
	}
};

class FetchSessionKeyAction final : public Action {
	const std::string username_;
	const dal::UserDAO& user_src_;
	boost::optional<std::string> key_;
	std::exception_ptr exception_;

public:
	FetchSessionKeyAction(std::string username, const dal::UserDAO& user_src)
	                      : username_(std::move(username)), user_src_(user_src) {}

	virtual void execute() override try {
		key_ = user_src_.session_key(username_);
	} catch(dal::exception) {
		exception_ = std::current_exception();
	}

	boost::optional<std::string> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(key_);
	}

	std::string username() {
		return username_;
	}
};

class StoreSessionAction final : public Action {
	User user_;
	const std::string ip_, key_;
	const dal::UserDAO& user_src_;
	std::exception_ptr exception_;

public:
	StoreSessionAction(User user, std::string ip, std::string key, const dal::UserDAO& user_src)
	                   : user_(std::move(user)), ip_(std::move(ip)), key_(std::move(key)),
	                     user_src_(user_src){}

	virtual void execute() override try {
		user_src_.record_last_login(user_, ip_); // todo - transaction for both of these calls
		user_src_.session_key(user_.username(), key_); // todo - user vs username, why?
	} catch(dal::exception) {
		exception_ = std::current_exception();
	}

	void rethrow_exception() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}
	}

	boost::optional<User> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(user_);
	}
};

class FetchUserAction final : public Action {
	const std::string username_;
	const dal::UserDAO& user_src_;
	boost::optional<User> user_;
	std::exception_ptr exception_;

public:
	FetchUserAction(std::string username, const dal::UserDAO& user_src)
	                : username_(std::move(username)), user_src_(user_src) {}

	virtual void execute() override try {
		user_ = user_src_.user(username_);
	} catch(dal::exception) {
		exception_ = std::current_exception();
	}

	boost::optional<User> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return std::move(user_);
	}

	std::string username() {
		return username_;
	}
};

} // ember