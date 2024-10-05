/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountClient.h"
#include <vector>

namespace ember {

namespace em = messaging::Accountv2;

AccountClient::AccountClient(spark::v2::Server& spark, log::Logger& logger)
	: services::Accountv2Client(spark),
	  logger_(logger) {
	connect("127.0.0.1", 8000); // temp
}

void AccountClient::on_link_up(const spark::v2::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	link_ = link;
}

void AccountClient::on_link_down(const spark::v2::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void AccountClient::locate_session(const std::uint32_t account_id, LocateCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::SessionLookupT msg {
		.account_id = account_id
	};

	send<em::SessionResponse>(msg, link_,
		[this, cb = std::move(cb)](auto link, auto message) {
			handle_locate_response(message, cb);
		}
	);
}

void AccountClient::locate_account_id(const std::string& username, AccountCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::LookupIDT msg {
		.account_name = username
	};

	send<em::AccountFetchResponse>(msg, link_,
		[this, cb = std::move(cb)](auto link, auto message) {
			handle_lookup_response(message, cb);
		}
	);
}

void AccountClient::handle_lookup_response(
	std::expected<const em::AccountFetchResponse*, spark::v2::Result> resp,
	const AccountCB& cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(!resp) {
		cb(messaging::Accountv2::Status::RPC_ERROR, {});
		return;
	}

	const auto msg = *resp;
	cb(msg->status(), msg->account_id());
}

void AccountClient::handle_locate_response(
	std::expected<const em::SessionResponse*, spark::v2::Result> resp,
	const LocateCB& cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(!resp) {
		cb(messaging::Accountv2::Status::RPC_ERROR, {});
		return;
	}
	
	const auto msg = *resp;

	if(!msg->key()) {
		cb(msg->status(), {});
		return;
	}

	auto key = Botan::BigInt::decode(msg->key()->data(), msg->key()->size());
	cb(msg->status(), std::move(key));
}

} // ember