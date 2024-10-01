/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "HelloServiceStub.h"

namespace ember {

class HelloService final : public services::HelloService {
	void handle_say_hello(const messaging::Hello::HelloRequest* msg,
	                      messaging::Hello::HelloReplyT& reply) override;

public:
	HelloService(spark::v2::Server& server);

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;
};

}