/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "NetworkSession.h"
#include <set>

namespace ember {

class SessionManager {
	std::set<SharedNetSession> sessions_;

public:
	void start(SharedNetSession session);
	void stop(SharedNetSession session);
	void stop_all();
};

} // ember