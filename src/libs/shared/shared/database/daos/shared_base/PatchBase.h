/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/Exception.h>
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace ember { namespace dal {

class PatchBase {
public:
	virtual ~PatchBase() = default;
};

}} // dal, ember