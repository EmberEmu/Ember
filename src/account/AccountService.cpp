/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"

namespace ember {

AccountService::AccountService(spark::v2::Server& spark, log::Logger& logger)
	: services::Accountv2Service(spark),
	  logger_(logger) {}

void AccountService::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up from {}", link.peer_banner);
}

void AccountService::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link down from {}", link.peer_banner);
}

std::optional<messaging::Accountv2::SessionResponseT> AccountService::handle_session_fetch(
	const messaging::Accountv2::SessionLookup* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<messaging::Accountv2::ResponseT> AccountService::handle_register_session(
	const messaging::Accountv2::RegisterSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<messaging::Accountv2::LookupIDResponseT> AccountService::handle_account_i_d_fetch(
	const messaging::Accountv2::LookupID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<messaging::Accountv2::ResponseT> AccountService::handle_disconnect_session(
	const messaging::Accountv2::DisconnectSession* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

std::optional<messaging::Accountv2::ResponseT> AccountService::handle_disconnect_by_i_d(
	const messaging::Accountv2::DisconnectID* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	return std::nullopt; 
}

} // ember