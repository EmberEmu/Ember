/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sessions.h"
#include <AccountServiceStub.h>
#include <logger/Logger.h>

namespace ember {

class AccountService final : public services::AccountService {
	Sessions& sessions_;
	log::Logger& logger_;

	std::optional<rpc::Account::SessionResponseT> handle_session_fetch(
		const rpc::Account::SessionLookup* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Account::RegisterResponseT> handle_register_session(
		const rpc::Account::RegisterSession* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Account::AccountFetchResponseT> handle_account_i_d_fetch(
		const rpc::Account::LookupID* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Account::DisconnectSessionResponseT> handle_disconnect_session(
		const rpc::Account::DisconnectSession* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Account::DisconnectResponseT> handle_disconnect_by_i_d(
		const rpc::Account::DisconnectID* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

public:
	AccountService(spark::v2::Server& spark, Sessions& sessions, log::Logger& logger);
};

} // ember