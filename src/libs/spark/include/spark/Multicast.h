/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/multicast/Receiver.h>
#include <spark/multicast/Sender.h>
#include <boost/asio.hpp>
#include <string>
#include <cstdint>

namespace ember { namespace spark {

class Service;

class Multicast {
	boost::asio::io_service& io_service_;
	boost::asio::signal_set signals_;
	Service& service_;
	multicast::Receiver receiver_;
	multicast::Sender sender_;

	void shutdown();

public:
	Multicast(Service& service, std::string address, std::uint16_t port);
};

}} // spark, ember