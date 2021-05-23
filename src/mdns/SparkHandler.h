/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Service.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::dns {

class SparkHandler final {
	std::unique_ptr<spark::Service> service_;

public:
	SparkHandler(std::unique_ptr<spark::Service> service);
	void shutdown();
};

} // dns, ember