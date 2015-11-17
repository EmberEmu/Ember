/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Utility.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <type_traits>

namespace ember { namespace spark { namespace detail {

std::vector<ServicesType> services_to_uint8(const std::vector<messaging::Service>& services) {
	std::vector<ServicesType> ret;

	for(auto& service : services) {
		ret.emplace_back(static_cast<ServicesType>(service));
	}

	return ret;
}

std::vector<messaging::Service> uint8_to_services(const std::vector<ServicesType>& services) {
	std::vector<messaging::Service> ret;

	for(auto& service : services) {
		ret.emplace_back(static_cast<messaging::Service>(service));
	}

	return ret;
}

}}} // detail, spark, ember