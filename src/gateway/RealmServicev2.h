/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/Realm.h>
#include <RealmStatusv2ServiceStub.h>
#include <logger/Logger.h>
#include <mutex>
#include <vector>

namespace ember {

class RealmServicev2 : public services::RealmStatusv2Service {
	std::vector<spark::v2::Link> links_;

	Realm realm_;
	log::Logger& logger_;
	std::mutex mutex;

	messaging::RealmStatusv2::RealmStatusT status();

	std::optional<messaging::RealmStatusv2::RealmStatusT> handle_status_request(
		const messaging::RealmStatusv2::RequestRealmStatus* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

	void broadcast_status();
	void set_online();
	void set_offline();

public:
	RealmServicev2(spark::v2::Server& server, Realm realm, log::Logger& logger);
};

} // ember