/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "LoginHandler.h"
#include "RealmList.h"
#include <logger/Logging.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/Metrics.h>

namespace ember {

class LoginHandlerBuilder {
	log::Logger* logger_;
	const Patcher& patcher_;
	const RealmList& realm_list_;
	const dal::UserDAO& user_dao_;
	Metrics* metrics_;

public:
	LoginHandlerBuilder(log::Logger* logger, const Patcher& patcher, const dal::UserDAO& user_dao,
	                    RealmList& realm_list, Metrics* metrics)
	                    : logger_(logger), patcher_(patcher), user_dao_(user_dao),
	                      realm_list_(realm_list), metrics_(metrics) {}

	LoginHandler create(const std::string& source) {
		return LoginHandler(source, user_dao_, patcher_, logger_, realm_list_, metrics_);
	}
};

} // ember