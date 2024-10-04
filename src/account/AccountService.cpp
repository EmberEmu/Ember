/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"

namespace ember {

AccountService::AccountService(spark::v2::Server& spark, Sessions& sessions, log::Logger& logger)
	: services::Accountv2Service(spark),
	  sessions_(sessions),
	  logger_(logger) {}

void AccountService::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up from {}", link.peer_banner);
}

void AccountService::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link down from {}", link.peer_banner);
}

std::optional<messaging::Accountv2::SessionResponseT>
AccountService::handle_session_fetch(
	const messaging::Accountv2::SessionLookup* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	messaging::Accountv2::SessionResponseT response;

	if(!msg->account_id()) {
		response.status = messaging::Accountv2::Status::ILLFORMED_MESSAGE;
		return response;
	}

	const auto session = sessions_.lookup_session(msg->account_id());
		
	if(!session) {
		response.status = messaging::Accountv2::Status::SESSION_NOT_FOUND;
		return response;
	}

	auto key = Botan::BigInt::encode(*session);

	response.status = messaging::Accountv2::Status::OK;
	response.account_id = msg->account_id();
	response.key = std::move(key);
	return response;
}

std::optional<messaging::Accountv2::RegisterResponseT>
AccountService::handle_register_session(
	const messaging::Accountv2::RegisterSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	messaging::Accountv2::RegisterResponseT response {
		.status = messaging::Accountv2::Status::OK
	};

	if(msg->key() && msg->account_id()) {
		Botan::BigInt key(msg->key()->data(), msg->key()->size());

		if(!sessions_.register_session(msg->account_id(), key)) {
			response.status = messaging::Accountv2::Status::ALREADY_LOGGED_IN;
		}
	} else {
		response.status = messaging::Accountv2::Status::ILLFORMED_MESSAGE;
	}

	return response;
}

std::optional<messaging::Accountv2::AccountFetchResponseT>
AccountService::handle_account_i_d_fetch(
	const messaging::Accountv2::LookupID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	messaging::Accountv2::AccountFetchResponseT response {
		.status = messaging::Accountv2::Status::OK,
		.account_id = 1 // todo, temp
	};

	return response; 
}

std::optional<messaging::Accountv2::DisconnectSessionResponseT>
AccountService::handle_disconnect_session(
	const messaging::Accountv2::DisconnectSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<messaging::Accountv2::DisconnectResponseT>
AccountService::handle_disconnect_by_i_d(
	const messaging::Accountv2::DisconnectID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

} // ember