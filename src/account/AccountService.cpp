/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"

namespace ember {

namespace em = messaging::Account;

AccountService::AccountService(spark::v2::Server& spark, Sessions& sessions, log::Logger& logger)
	: services::AccountService(spark),
	  sessions_(sessions),
	  logger_(logger) {}

void AccountService::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up from {}", link.peer_banner);
}

void AccountService::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link down from {}", link.peer_banner);
}

std::optional<em::SessionResponseT>
AccountService::handle_session_fetch(
	const em::SessionLookup* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::SessionResponseT response;

	if(!msg->account_id()) {
		response.status = em::Status::ILLFORMED_MESSAGE;
		return response;
	}

	const auto session = sessions_.lookup_session(msg->account_id());
		
	if(!session) {
		response.status = em::Status::SESSION_NOT_FOUND;
		return response;
	}

	auto key = Botan::BigInt::encode(*session);

	response.status = em::Status::OK;
	response.account_id = msg->account_id();
	response.key = std::move(key);
	return response;
}

std::optional<em::RegisterResponseT>
AccountService::handle_register_session(
	const em::RegisterSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::RegisterResponseT response {
		.status = em::Status::OK
	};

	if(msg->key() && msg->account_id()) {
		Botan::BigInt key(msg->key()->data(), msg->key()->size());

		if(!sessions_.register_session(msg->account_id(), key)) {
			response.status = em::Status::ALREADY_LOGGED_IN;
		}
	} else {
		response.status = em::Status::ILLFORMED_MESSAGE;
	}

	return response;
}

std::optional<em::AccountFetchResponseT>
AccountService::handle_account_i_d_fetch(
	const em::LookupID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::AccountFetchResponseT response {
		.status = em::Status::OK,
		.account_id = 1 // todo, temp
	};

	return response; 
}

std::optional<em::DisconnectSessionResponseT>
AccountService::handle_disconnect_session(
	const em::DisconnectSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<em::DisconnectResponseT>
AccountService::handle_disconnect_by_i_d(
	const em::DisconnectID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

} // ember