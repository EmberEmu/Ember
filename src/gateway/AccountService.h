/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Account_generated.h"
#include <spark/Service.h>
#include <spark/ServiceDiscovery.h>
#include <logger/LoggerFwd.h>
#include <shared/util/UTF8String.h>
#include <botan/bigint.h>
#include <boost/uuid/uuid_generators.hpp>
#include <functional>
#include <memory>
#include <cstdint>

namespace ember {

class AccountService final : public spark::EventHandler {
public:
	using RegisterCB = std::function<void(messaging::account::Status)>;
	using SessionLocateCB = std::function<void(messaging::account::Status, Botan::BigInt)>;
	using IDLocateCB = std::function<void(messaging::account::Status, std::uint32_t)>;

private:
	spark::Service& spark_;
	spark::ServiceDiscovery& s_disc_;
	log::Logger* logger_;
	std::unique_ptr<spark::ServiceListener> listener_;
	spark::Link link_;
	
	void service_located(const messaging::multicast::LocateResponse* message);

	void handle_register_reply(const spark::Link& link, std::optional<spark::Message>& root,
	                           const RegisterCB& cb) const;

	void handle_locate_reply(const spark::Link& link, std::optional<spark::Message>& root,
	                         const SessionLocateCB& cb) const;

	void handle_id_locate_reply(const spark::Link& link, std::optional<spark::Message>& root,
	                            const IDLocateCB& cb) const;

public:
	AccountService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger);
	~AccountService();

	void on_message(const spark::Link& link, const spark::Message& message) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;

	void locate_session(std::uint32_t account_id, SessionLocateCB cb) const;
	void locate_account_id(const utf8_string& username, IDLocateCB cb) const;
};

} // ember