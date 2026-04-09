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

void enter(ClientContext& ctx) {
	if(ctx.realm_rpc.online()) {
		ctx.handler.state_update(ClientState::cs_authenticating);
	} else {
		protocol::smsg_auth_response response;
		response->result = protocol::Result::auth_unavailable;
		ctx.handler.send(response);
		ctx.handler.state_update(ClientState::cs_session_closed);
	}
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	CLIENT_DEBUG(ctx, "Unexpected packet in connected state");
}

void handle_event(ClientContext& ctx, const Event& event) {
	CLIENT_DEBUG(ctx, "Unexpected event in connected state");
}

void exit(ClientContext& ctx) {
	// nothing to do
}

} // connected, realm, ember
