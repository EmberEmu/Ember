/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sessions.h"
#include "Account_generated.h"
#include <spark/Service.h>
#include <spark/Helpers.h>
#include <logger/Logging.h>
#include <boost/optional.hpp>
#include <cstdint>

namespace ember {

class Service final : public spark::EventHandler {
	Sessions& sessions_;
	spark::Service& spark_;
	spark::ServiceDiscovery& discovery_;
	log::Logger* logger_;
	std::unordered_map<messaging::core::Opcode, spark::LocalDispatcher> handlers_;

	void register_session(const spark::Link& link, const spark::Message& message);
	void locate_session(const spark::Link& link, const spark::Message& message);
	void send_locate_reply(const spark::Link& link, const boost::optional<Botan::BigInt>& key);
	void account_lookup(const spark::Link& link, const spark::Message& message);
	void send_register_reply(const spark::Link& link, messaging::account::Status status);

public:
	Service(Sessions& sessions, spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger);
	~Service();

	void on_message(const spark::Link& link, const spark::Message& message) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;
};

} // ember