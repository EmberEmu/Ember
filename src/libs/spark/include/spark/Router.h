/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Router.h>
#include <unordered_map>

namespace ember { namespace spark {

class Router {
	std::unordered_map<int, int> handlers_;

public:
	void register_inbound();
	void register_outbound();
};

}} // spark, ember