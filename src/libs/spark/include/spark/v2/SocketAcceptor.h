/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>

namespace ember::spark::v2 {

class SocketAcceptor {
public:
	virtual void accept(boost::asio::ip::tcp::socket socket) = 0;
	~SocketAcceptor() = default;
};

} // spark, ember