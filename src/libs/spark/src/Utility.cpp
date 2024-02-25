/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Utility.h>

namespace ember::spark::inline v1::detail {

std::vector<ServicesType> services_to_underlying(const std::vector<messaging::Service>& services) {
	std::vector<ServicesType> ret;

	for(auto& service : services) {
		ret.emplace_back(static_cast<ServicesType>(service));
	}

	return ret;
}

std::vector<messaging::Service> underlying_to_services(const std::vector<ServicesType>& services) {
	std::vector<messaging::Service> ret;

	for(auto& service : services) {
		ret.emplace_back(static_cast<messaging::Service>(service));
	}

	return ret;
}

} // detail, spark, ember