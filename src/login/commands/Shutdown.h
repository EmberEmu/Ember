/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Registry.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <functional>

namespace ember::shutdown {

enum State {
	pending,
	not_pending
};

using OnInitiate = std::function<void(std::chrono::seconds)>;
using OnCancel = std::function<void(State)>;
using OnExpire = std::function<void()>;
using OnRemaining = std::function<void(State, std::chrono::seconds)>;

struct Handlers {
	OnInitiate on_initiate;
	OnCancel on_cancel;
	OnExpire on_expire;
	OnRemaining on_remaining;
};

void register_command(commands::Registry& registry, boost::asio::io_context& ioc, Handlers handlers);

} // shutdown, ember