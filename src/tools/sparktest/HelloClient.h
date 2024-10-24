/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "HelloClientStub.h"

namespace ember {

class HelloClient final : public services::HelloClient {
	spark::v2::Server& spark_;

	void say_hello(const spark::v2::Link& link);
	void say_hello_tracked(const spark::v2::Link& link);

	void handle_say_hello_response(
		const spark::v2::Link& link,
		const rpc::Hello::HelloReply& msg
	) override;

	void handle_tracked_reply(
		const spark::v2::Link& link,
		std::expected<const rpc::Hello::HelloReply*, spark::v2::Result> msg
	);

public:
	HelloClient(spark::v2::Server& spark);

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;
};

}