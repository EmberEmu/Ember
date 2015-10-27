/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginSession.h"
#include "LoginHandlerBuilder.h"
#include <memory>

namespace ember {

LoginSession::LoginSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	                       log::Logger* logger, const LoginHandlerBuilder& builder)
						   : handler_(builder.create(*this)), logger_(logger),
                             NetworkSession(sessions, std::move(socket), logger) { }

void LoginSession::handle_packet(spark::Buffer& buffer) {
	LOG_WARN(logger_) << "Handling packet" << LOG_ASYNC;
}

} // ember
