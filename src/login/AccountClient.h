/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <Accountv2ClientStub.h>
#include <logger/Logger.h>
#include <spark/v2/Server.h>
#include <srp6/Util.h>
#include <functional>
#include <cstdint>

namespace ember {

class AccountClient final : public services::Accountv2Client {
public:
	using RegisterCB = std::function<void(messaging::Accountv2::Status)>;
	using LocateCB = std::function<void(messaging::Accountv2::Status, Botan::BigInt)>;

private:
	log::Logger& logger_;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

public:
	AccountClient(spark::v2::Server& spark, log::Logger& logger);

	void register_session(std::uint32_t account_id, const srp6::SessionKey& key, RegisterCB cb) const;
	void locate_session(std::uint32_t account_id, LocateCB cb) const;
};

} // ember