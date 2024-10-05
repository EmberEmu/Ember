/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <AccountClientStub.h>
#include <logger/Logger.h>
#include <spark/v2/Server.h>
#include <botan/bigint.h>
#include <functional>
#include <cstdint>

namespace ember {

class AccountClient final : public services::AccountClient {
public:
	using LocateCB = std::function<void(rpc::Account::Status, Botan::BigInt)>;
	using AccountCB = std::function<void(rpc::Account::Status, std::uint32_t)>;

private:
	log::Logger& logger_;
	spark::v2::Link link_;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

	void handle_locate_response(
		std::expected<const rpc::Account::SessionResponse*, spark::v2::Result> res,
		const LocateCB& cb
	) const;

	void handle_lookup_response(
		std::expected<const rpc::Account::AccountFetchResponse*, spark::v2::Result> res,
		const AccountCB& cb
	) const;

public:
	AccountClient(spark::v2::Server& spark, log::Logger& logger);

	void locate_session(std::uint32_t account_id, LocateCB cb) const;
	void locate_account_id(const std::string& username, AccountCB cb) const;
};

} // ember