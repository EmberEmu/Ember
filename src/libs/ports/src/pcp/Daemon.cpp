/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Daemon.h>

namespace ember::ports {

void Daemon::add_mapping(MapRequest request, ResultHandler&& handler) {
	//
}

void Daemon::delete_mapping(std::uint16_t internal_port, Protocol protocol,
                            ResultHandler&& handler) {
	//
}



} // natpmp, ports, ember
