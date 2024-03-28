/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "HelloService.h"

namespace ember {

HelloService::HelloService(spark::v2::Server& server)
	: services::HelloService(server) {}

void HelloService::on_link_up(const spark::v2::Link& link) {

}

void HelloService::on_link_down(const spark::v2::Link& link) {

}

}