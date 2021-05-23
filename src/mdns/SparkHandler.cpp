/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SparkHandler.h"

namespace ember::dns {

SparkHandler::SparkHandler(std::unique_ptr<spark::Service> service, log::Logger* logger)
	: service_(std::move(service)), logger_(logger) {}

void SparkHandler::shutdown() {
	service_->shutdown();
}

void SparkHandler::on_message(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void SparkHandler::on_link_up(const spark::Link& link) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void SparkHandler::on_link_down(const spark::Link& link) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

} // dns, ember