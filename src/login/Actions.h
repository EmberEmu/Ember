/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AccountService.h"
#include "grunt/Packet.h"
#include <shared/database/objects/User.h>
#include <shared/database/daos/UserDAO.h>
#include <boost/optional.hpp>
#include <exception>
#include <future>
#include <string>
#include <utility>
#include <cstdint>

namespace ember {

class Action {
protected:
	Action() = default;

public:
	virtual void execute() = 0;
	virtual ~Action() = default;
};

class RegisterSessionAction final : public Action {
	const AccountService& account_svc_;
	std::string account_;
	srp6::SessionKey key_;

	std::promise<messaging::account::Status> promise_;
	messaging::account::Status res_;
	std::exception_ptr exception_;

	std::future<messaging::account::Status> do_register() {
		account_svc_.register_session(account_, key_, [&](messaging::account::Status res) {
			promise_.set_value(res);
		});

		return promise_.get_future();
	}

public:
	RegisterSessionAction(const AccountService& account_svc, std::string account, srp6::SessionKey key)
	                      : account_svc_(account_svc), account_(std::move(account)), key_(key) { }

	virtual void execute() override try {
		res_ = do_register().get();
	} catch(const std::exception&) {
		exception_ = std::current_exception();
	}

	messaging::account::Status get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return res_;
	}
};

class FetchSessionKeyAction final : public Action {
	const AccountService& account_svc_;
	std::string account_;
	Botan::BigInt key_;
	std::exception_ptr exception_;

	std::promise<std::pair<messaging::account::Status, Botan::BigInt>> promise_;
	std::pair<messaging::account::Status, Botan::BigInt> res_;

	auto do_fetch() {
		account_svc_.locate_session(account_, [&](messaging::account::Status res,
		                            Botan::BigInt key) {
			promise_.set_value({res, key});
		});

		return promise_.get_future();
	}

public:
	FetchSessionKeyAction(const AccountService& account_svc, std::string account)
	                      : account_svc_(account_svc), account_(std::move(account)) {}

	virtual void execute() override try {
		res_ = do_fetch().get();
	} catch(const std::exception&) {
		exception_ = std::current_exception();
	}

	auto get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return res_;
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
	} catch(const dal::exception&) {
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

class FetchCharacterCounts final : public Action {
	const std::uint32_t user_id_;
	const dal::UserDAO& user_src_;
	std::unordered_map<std::uint32_t, std::uint32_t> counts_;
	bool reconnect_;
	std::exception_ptr exception_;

public:
	FetchCharacterCounts(std::uint32_t user_id, const dal::UserDAO& user_src, bool reconnect = false)
	                     : user_id_(user_id), user_src_(user_src), reconnect_(reconnect) {}

	virtual void execute() override try {
		counts_ = user_src_.character_counts(user_id_);
	} catch(const dal::exception&) {
		exception_ = std::current_exception();
	}

	std::unordered_map<std::uint32_t, std::uint32_t> get_result() {
		if(exception_) {
			std::rethrow_exception(exception_);
		}

		return counts_;
	}

	bool reconnect() {
		return reconnect_;
	}
};

} // ember