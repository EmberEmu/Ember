/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "EventDispatcher.h"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>

namespace ember::realm {

void EventDispatcher::exec(const ClientIdent& client, auto work) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::post(*service, [&, client, work = std::move(work)] {
		if(auto handler = locate_handler(client)) {
			work();
		} else {
			LOG_DEBUG(logger_, "Client disconnected, work discarded");
		}
	});
}


} // realm, ember