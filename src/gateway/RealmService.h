/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/Realm.h>
#include <RealmServiceStub.h>
#include <logger/Logger.h>
#include <mutex>
#include <vector>

namespace ember {

class RealmService : public services::RealmService {
	std::vector<spark::v2::Link> links_;

	Realm realm_;
	log::Logger& logger_;
	std::mutex mutex;

	messaging::Realm::StatusT status();

	std::optional<messaging::Realm::StatusT> handle_get_status(
		const messaging::Realm::RequestStatus* msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;
	void broadcast_status();

public:
	RealmService(spark::v2::Server& server, Realm realm, log::Logger& logger);

	void set_online();
	void set_offline();
};

} // ember