/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RequestHandler.h"
#include <logger/Logger.h>

namespace ember::dns {

RequestHandler::RequestHandler(log::Logger* logger) : logger_(logger) {}

void RequestHandler::shutdown() {
}

void RequestHandler::on_message(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void RequestHandler::on_link_up(const spark::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

void RequestHandler::on_link_down(const spark::Link& link) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
}

} // dns, ember