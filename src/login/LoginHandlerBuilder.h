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
#include <memory>
#include <utility>

namespace ember {

class Patcher;
class RealmList;

class LoginHandlerBuilder {
	log::Logger* logger_;
	const Patcher& patcher_;
	const RealmList& realm_list_;
	const dal::UserDAO& user_dao_;

public:
	LoginHandlerBuilder(log::Logger* logger, const Patcher& patcher, const dal::UserDAO& user_dao,
	                    RealmList& realm_list) : logger_(logger), patcher_(patcher),
	                    user_dao_(user_dao), realm_list_(realm_list) {}

	LoginHandler create(const NetworkSession& net_session, std::string source) const {
		return { net_session, user_dao_, patcher_, logger_, realm_list_, std::move(source) };
	}
};

} // ember