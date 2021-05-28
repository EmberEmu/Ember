/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/PeerHandler.h>
#include <spark/v2/PeerConnection.h>
#include <spark/v2/Dispatcher.h>
#include <concepts>
#include <utility>

namespace ember::spark::v2 {

template<typename Handler, typename Connection, typename Socket>
class RemotePeer final : public Dispatcher {
	Handler handler_;
	Connection conn_;

public:
	explicit RemotePeer(Socket socket)
		: handler_(*this), conn_(*this, std::move(socket)) {}

	void send() {

	}

	void receive() {

	}
};

} // spark, ember