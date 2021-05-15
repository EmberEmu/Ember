 /*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Server.h"
#include "Socket.h"
#include "Parser.h"
#include <shared/util/FormatPacket.h>
#include <iostream>

namespace ember::dns {

Server::Server(Socket& socket, Parser& parser, log::Logger* logger)
               : socket_(socket), parser_(parser), logger_(logger) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
    socket_.register_handler(this);
}

void Server::handle_query(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void Server::handle_response(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void Server::handle_datagram(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

    if(Parser::validate(datagram) != Result::VALIDATE_OK) {
        LOG_DEBUG(logger_) << "Validation failed with error code" << LOG_SYNC;
        return;
    }

	std::cout << util::format_packet((unsigned char*)datagram.data(), datagram.size()) << "\n";

    const auto header = Parser::header_overlay(datagram);
    const auto flags = Parser::extract_flags(*header);
    
    if(flags.qr == 0) {
        handle_query(datagram);
    } else {
        handle_response(datagram);
    }
}

} // dns, ember