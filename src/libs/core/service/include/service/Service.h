/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <service/Config.h>
#include <boost/program_options/variables_map.hpp>

namespace ember {

class Service {
public:
	Service() = default;
	virtual ~Service() = default;

	virtual int run(const boost::program_options::variables_map&) = 0;
	virtual void stop() = 0;
};

} // ember