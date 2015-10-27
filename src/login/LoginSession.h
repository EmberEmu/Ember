/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "NetworkSession.h"
#include "LoginHandler.h"
#include <spark/Buffer.h>
#include <logger/Logging.h>
#include <memory>

namespace ember {

class LoginHandlerBuilder;

class LoginSession final : public NetworkSession {
public:
	LoginHandler handler_;
	log::Logger* logger_;

	LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	             log::Logger* logger, const LoginHandlerBuilder& builder);

	void handle_packet(spark::Buffer& buffer) override;
};

} // ember