/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/ServiceListener.h>
#include <spark/ServiceDiscovery.h>

namespace ember::spark::inline v1 {

ServiceListener::~ServiceListener() {
	sd_client_->remove_listener(this);
}

void ServiceListener::search() {
	sd_client_->locate_service(service_);
}

} // spark, ember