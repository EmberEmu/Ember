/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "RealmList.h"
#include <logger/Logger.h>
#include <RealmStatusv2ClientStub.h>
#include <unordered_map>

namespace ember {

class RealmClient final : public services::RealmStatusv2Client {
	RealmList& realmlist_;
	log::Logger& logger_;
	std::unordered_map<std::string, std::uint32_t> realms_;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

	void request_realm_status(const spark::v2::Link& link);
	void mark_realm_offline(const spark::v2::Link& link);

	void handle_status_request_response(
		const spark::v2::Link& link,
		const ember::messaging::RealmStatusv2::RealmStatus* msg
	) override;

public:
	RealmClient(spark::v2::Server& server, RealmList& realmlist, log::Logger& logger);
};

} // ember