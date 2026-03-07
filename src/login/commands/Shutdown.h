/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Registry.h>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>

namespace ember {

namespace shutdown {

using OnInitiate = std::function<void(std::chrono::seconds)>;
using OnCancel = std::function<void()>;
using OnExpire = std::function<void()>;

} // shutdown

void register_shutdown_command(commands::Registry& registry,
                               boost::asio::steady_timer& timer,
                               bool& pending_flag,
                               shutdown::OnInitiate on_initiate,
                               shutdown::OnCancel on_cancel,
                               shutdown::OnExpire handler);

} // ember