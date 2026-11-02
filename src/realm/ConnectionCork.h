/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"

namespace ember::realm {

class ConnectionCork {
	ClientConnection& connection_;
	CorkToken token_;
	bool released_;

public:
	ConnectionCork(ClientConnection& connection)
		: connection_(connection)
		, token_(connection.cork())
		, released_(false) {}

	void release() {
		if(released_) {
			return;
		}

		connection_.uncork(token_);
		released_ = true;

	}

	~ConnectionCork() {
		release();
	}
};


} // realm, ember