/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "HelloClient.h"
#include <iostream>

namespace ember {

HelloClient::HelloClient(spark::v2::Server& spark)
	: services::HelloClient(spark),
	  spark_(spark) {
	connect("127.0.0.1", 8000);
}

void HelloClient::on_link_up(const spark::v2::Link& link) {
	std::cout << "Link up\n";
}

void HelloClient::on_link_down(const spark::v2::Link& link) {

}

}