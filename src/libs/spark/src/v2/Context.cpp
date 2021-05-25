/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Context.h>

namespace ember::spark::v2 {

Context::Context(boost::asio::io_context& context, const std::string& iface,
                 const std::uint16_t port, log::Logger* logger)
	: context_(context), logger_(logger) {}

void Context::register_service(spark::v2::Service* service) {

}

void Context::shutdown() {

}

} // spark, ember