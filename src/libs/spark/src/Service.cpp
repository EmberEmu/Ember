/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Service.h>

namespace ember { namespace spark {

Service::Service(boost::asio::io_service& service, std::string host, std::uint16_t port,
                 log::Logger* logger, log::Filter filter)
                 : service_(service), filter_(filter) {
}

}}