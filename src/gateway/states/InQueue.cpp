/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "InQueue.h"
#include "Authentication.h"
#include "../EventTypes.h"
#include "../Locator.h"
#include "../ClientHandler.h"
#include "../ClientConnection.h"
#include "../RealmQueue.h"
#include <logger/Logging.h>
#include <game_protocol/server/SMSG_AUTH_RESPONSE.h>
#include <gsl/gsl_util>

namespace ember::queue {

void handle_queue_update(ClientContext* ctx, const QueuePosition* event) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::SMSG_AUTH_RESPONSE packet;
	packet.result = protocol::Result::AUTH_WAIT_QUEUE;
	packet.queue_position = gsl::narrow_cast<std::uint32_t>(event->position);
	ctx->connection->send(packet);
}

void handle_queue_success(ClientContext* ctx, const QueueSuccess* event) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;
	authentication::auth_success(ctx, event->packet);
}

void enter(ClientContext* ctx) {
	LOG_DEBUG_GLOB << ctx->account_name << " added to queue" << LOG_ASYNC;
}

void handle_event(ClientContext* ctx, const Event* event) {
	switch(event->type) {
		case EventType::QUEUE_UPDATE_POSITION:
			handle_queue_update(ctx, static_cast<const QueuePosition*>(event));
			break;
		case EventType::QUEUE_SUCCESS:
			handle_queue_success(ctx, static_cast<const QueueSuccess*>(event));
			break;
		default:
			break;
	}
}

void handle_packet(ClientContext* ctx, protocol::ClientOpcode opcode) {
	ctx->handler->packet_skip(*ctx->buffer, opcode);
}

void exit(ClientContext* ctx) {
	LOG_DEBUG_GLOB << ctx->account_name << " removed from queue" << LOG_ASYNC;

	if(ctx->state == ClientState::SESSION_CLOSED) {
		Locator::queue()->dequeue(ctx->handler->uuid());
	}
}

} // queue, ember