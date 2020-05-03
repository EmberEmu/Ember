/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldForwarder.h"
#include "../Routing.h"
#include "../FilterTypes.h"
#include <logger/Logging.h>

namespace ember::world {

void route_packet(ClientContext* context, protocol::ClientOpcode opcode, Route route);

void enter(ClientContext* context) {

}

void handle_packet(ClientContext* context, protocol::ClientOpcode opcode) {
	if(auto it = cmsg_routes.find(opcode); it != cmsg_routes.end()) {
		auto& [op, route] = *it;
		route_packet(context, opcode, route);
	} else {
		LOG_DEBUG_FILTER_GLOB(LF_NETWORK) << "Received unroutable packet, "
			<< protocol::to_string(opcode)
			<< " from " << context->account_name << LOG_ASYNC;
	}
}

void handle_event(ClientContext* ctx, const Event* event) {

}

void route_packet(ClientContext* context, protocol::ClientOpcode opcode, Route route) {
	// all dressed up and nowhere to go
}

void exit(ClientContext* context) {
	//queue_service_temp->free_slot();
}

} // world, ember