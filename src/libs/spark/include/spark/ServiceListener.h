/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Multicast_generated.h"
#include "Services_generated.h"
#include <spark/Common.h>

namespace ember::spark::inline v1 {

typedef std::function<void(const messaging::multicast::LocateResponse*)> LocateCallback;

class ServiceDiscovery;

class ServiceListener {
	ServiceDiscovery* sd_client_;
	const messaging::Service service_;
	LocateCallback cb_;

public:
	ServiceListener(ServiceDiscovery* sd, messaging::Service service, LocateCallback cb)
		: sd_client_(sd), service_(service), cb_(cb) {}

	messaging::Service service() const { return service_; }

	void search();
	~ServiceListener();

	friend class ServiceDiscovery;
};

} // spark, ember