/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/temp/Multicast_generated.h>

namespace ember { namespace spark {

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

}} // spark, ember