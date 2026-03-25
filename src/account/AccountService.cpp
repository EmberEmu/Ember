/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include <logger/Logger.h>

namespace ember {

using namespace rpc::Account;
using namespace spark;

AccountService::AccountService(Server& spark, AccountHandler& handler, Sessions& sessions, log::Logger& logger)
	: handler_(handler), 
	  services::AccountService(spark),
	  sessions_(sessions),
	  logger_(logger) {}

void AccountService::on_link_up(const Link& link) {
	LOG_DEBUG(logger_, "Link up from {}", link.peer_banner);
}

void AccountService::on_link_down(const Link& link) {
	LOG_DEBUG(logger_, "Link down from {}", link.peer_banner);
}

std::optional<SessionResponseT>
AccountService::handle_session_fetch(const SessionLookup& msg, const Link& link, const Token& token) {
	LOG_TRACE(logger_, log_func);

	SessionResponseT response;

	if(!msg.account_id()) {
		response.status = Status::illformed_message;
		return response;
	}

	const auto session = sessions_.lookup_session(msg.account_id());
		
	if(!session) {
		response.status = Status::session_not_found;
		return response;
	}

	auto key = session->serialize();

	response.status = Status::ok;
	response.account_id = msg.account_id();
	response.key = std::move(key);
	return response;
}

std::optional<RegisterResponseT>
AccountService::handle_register_session(const RegisterSession& msg,	const Link& link, const Token& token) {
	LOG_TRACE(logger_, log_func);

	RegisterResponseT response {
		.status = Status::ok
	};

	if(msg.key() && msg.account_id()) {
		Botan::BigInt key(msg.key()->data(), msg.key()->size());

		if(!sessions_.register_session(msg.account_id(), key)) {
			response.status = Status::already_logged_in;
		}
	} else {
		response.status = Status::illformed_message;
	}

	return response;
}

std::optional<AccountFetchResponseT>
AccountService::handle_account_id_fetch(const LookupID& msg, const Link& link, const Token& token) {
	LOG_TRACE(logger_, log_func);
	
	if(!msg.account_name()) {
		return AccountFetchResponseT {
			.status = Status::illformed_message
		};
	}

	handler_.lookup_id(msg.account_name()->str(), [&, link, token](auto result) {
		if(!result) {
			AccountFetchResponseT response {
				.status = Status::unknown_error
			};

			send(response, link, token);
			return;
		}

		const auto id = *result;

		AccountFetchResponseT response {
			.status = id? Status::ok : Status::account_not_found,
			.account_id = id? *id : 0
		};
		
		send(response, link, token);
	});

	return std::nullopt;
}

std::optional<DisconnectSessionResponseT>
AccountService::handle_disconnect_by_session(const DisconnectSession& msg, const Link& link, const Token& token) {
	LOG_TRACE(logger_, log_func);
	return std::nullopt;
}

std::optional<DisconnectResponseT>
AccountService::handle_disconnect_by_id(const DisconnectID& msg, const Link& link, const Token& token) {
	LOG_TRACE(logger_, log_func);
	return std::nullopt; 
}

} // ember