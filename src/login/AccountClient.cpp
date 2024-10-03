/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountClient.h"

namespace ember {

AccountClient::AccountClient(spark::v2::Server& spark, log::Logger& logger)
	: services::Accountv2Client(spark),
	  logger_(logger) {
	connect("127.0.0.1", 8000); // temp
}

void AccountClient::on_link_up(const spark::v2::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void AccountClient::on_link_down(const spark::v2::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void AccountClient::locate_session(const std::uint32_t account_id, LocateCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void AccountClient::register_session(const std::uint32_t account_id,
                                     const srp6::SessionKey& key,
                                     RegisterCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

} // ember