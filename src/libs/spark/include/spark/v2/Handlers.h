/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <unordered_map>
#include <spark/v2/Service.h>

namespace ember::spark::v2 {

class Handlers {
	std::unordered_map<std::string, Service*> services_;
	std::unordered_map<std::string, Service*> clients_;

public:

};

}