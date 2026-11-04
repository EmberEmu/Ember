/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

/*
 * These aliases exist so we can avoid relying on Asio's
 * default polymorphic executor (any_io_executor) and
 * having to pay a performance penalty for a feature we
 * don't need
 */
namespace ember {

using Executor = boost::asio::io_context::executor_type;

namespace tcp {

using Socket = boost::asio::basic_stream_socket<boost::asio::ip::tcp, Executor>;
using StrandSocket = boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::strand<Executor>>;
using Acceptor = boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, Executor>;

} // tcp

} // ember