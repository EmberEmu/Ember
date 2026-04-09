/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Connected.h"
#include "ClientContext.h"
#include "../RealmService.h"
#include "../ClientHandler.h"

namespace ember::realm::connected {

/*
 * If the realm is pretending to be offline, disconnect the client.
 * 
 * Big caveat: doing this will kick it back to login rather than the realm list,
 * but I can't find a way that allows for displaying a message on the realm
 * list. I think the ability to do this properly was abandoned beyond earlier protocol
 * versions that can be found in alpha clients.
 *
 * If the connection isn't closed, no message is displayed and the client will
 * crash when attempting to reconnect. The client won't close the connection
 * on its own unless certain conditions are met.
 */
void enter(ClientContext& ctx) {
	if(ctx.realm_rpc.online()) {
		ctx.handler.state_update(ClientState::cs_authenticating);
	} else {
		protocol::smsg_auth_response response;
		response->result = protocol::Result::realm_list_realm_not_found;
		ctx.handler.send(response);
		ctx.handler.close();
	}
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	CLIENT_DEBUG(ctx, "Unexpected packet in connected state");
	ctx.handler.skip(*ctx.stream);
}

void handle_event(ClientContext& ctx, const Event& event) {
	CLIENT_DEBUG(ctx, "Unexpected event in connected state");
}

void exit(ClientContext& ctx) {
	// nothing to do
}

} // connected, realm, ember
