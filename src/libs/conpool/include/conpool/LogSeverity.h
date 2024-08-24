/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#undef ERROR 

namespace ember::connection_pool {

enum class Severity {
	DEBUG, INFO, WARN, ERROR, FATAL
};

} // connection_pool, ember