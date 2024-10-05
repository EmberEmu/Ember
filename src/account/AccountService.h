/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sessions.h"
#include <Accountv2ServiceStub.h>
#include <logger/Logger.h>

namespace ember {

namespace em = messaging::Accountv2;

class AccountService final : public services::Accountv2Service {
	Sessions& sessions_;
	log::Logger& logger_;

	std::optional<em::SessionResponseT> handle_session_fetch(
		const em::SessionLookup* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<em::RegisterResponseT> handle_register_session(
		const em::RegisterSession* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<em::AccountFetchResponseT> handle_account_i_d_fetch(
		const em::LookupID* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<em::DisconnectSessionResponseT> handle_disconnect_session(
		const em::DisconnectSession* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<em::DisconnectResponseT> handle_disconnect_by_i_d(
		const em::DisconnectID* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

public:
	AccountService(spark::v2::Server& spark, Sessions& sessions, log::Logger& logger);
};

} // ember