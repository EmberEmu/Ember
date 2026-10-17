/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/CommandSink.h>

namespace ember::log {

CommandSink::CommandSink(std::string name, Severity severity, Filter filter)
	: Sink(std::move(name), std::string(type()), severity, filter) {}

} // log, ember