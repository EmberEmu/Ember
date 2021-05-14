/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Handler.h"
#include <logger/Logging.h>

namespace ember::dns {

class Socket;

class Server : public Handler {
    Socket& socket_;
    log::Logger* logger_;

    void handle_query(std::span<const std::byte> datagram);
    void handle_response(std::span<const std::byte> datagram);
    void handle_datagram(std::span<const std::byte> datagram) override;

public:
    Server(Socket& socket, log::Logger* logger);
};

} // dns, ember