/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginHandler.h"
#include <logger/Logging.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/Metrics.h>
#include <utility>

namespace ember {

class Patcher;
class RealmList;
class Metrics;
class AccountService;
class IntegrityData;

class LoginHandlerBuilder {
	log::Logger* logger_;
	const Patcher& patcher_;
	const RealmList& realm_list_;
	const dal::UserDAO& user_dao_;
	const AccountService& acct_svc_;
	const IntegrityData* exe_data_;
	Metrics& metrics_;

public:
	LoginHandlerBuilder(log::Logger* logger, const Patcher& patcher, const IntegrityData* exe_data,
	                    const dal::UserDAO& user_dao, const AccountService& acct_svc, RealmList& realm_list, Metrics& metrics)
	                    : logger_(logger), patcher_(patcher), user_dao_(user_dao), acct_svc_(acct_svc),
	                      realm_list_(realm_list), metrics_(metrics), exe_data_(exe_data) {}

	LoginHandler create(std::string source) const {
		return { user_dao_, acct_svc_, patcher_, exe_data_, logger_, realm_list_, std::move(source), metrics_ };
	}
};

} // ember