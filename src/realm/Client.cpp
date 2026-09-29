/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Client.h"
#include "ClientSlots.h"
#include "EventDispatcher.h"
#include "packet_log/Helper.h"
#include <logger/Logger.h>
#include <cassert>

using namespace std::string_view_literals;

namespace ember::realm {

Client::Client(ClientHandler handler, ClientConnection connection,
               std::size_t index, EventDispatcher& dispatcher,
               log::Logger& logger)
	: handler_(std::move(handler))
	, connection_(std::move(connection))
	, dispatcher_(dispatcher)
	, logger_(logger)
	, running_(false)
	, index_(index) {}

bool Client::handle_self_event(const Event& event) {
	using enum EventType;

	switch(event.type) {
		case kick_self:
			handle_kick();
			return true;
		case packet_log_enable:
			packet_log_start();
			return true;
		case packet_log_disable:
			connection_.packet_log_stop();
			return true;
		case request_stop:
			handle_request_stop();
			return true;
		default:
			return false;
	}
}

void Client::handle_kick() {
	LOG_DEBUG(logger_, "{}, kicking self", handler_.whoami());
	stop();
}

void Client::handle_request_stop() {
	LOG_DEBUG(logger_, "{}, requested stop", handler_.whoami());
	stop();
}

void Client::packet_log_start() {
	handler_.log_redirect_stop(); // avoid generating an infinite packet loop
	
	auto plogger = create_packet_logger(
		"realm", connection_.remote_address(), ident_.to_string(), logger_, false
	);

	if(!plogger) {
		LOG_ERROR(logger_, "Packet logger creation failed!");
		return;
	}

	connection_.packet_log_start(std::move(plogger));
}

void Client::handle_event(const Event& event) {
	if(handle_self_event(event)) {
		return;
	}

	handler_.handle_event(event);
}

void Client::start() {
	if(running_.exchange(true)) {
		assert(running_);
		return;
	}

	ClientSlots::update_peak();

	ident_ = dispatcher_.register_client(this, index_);

	// Referencing each other like this is fine because they won't start running until we return
	// - that's assuming we're running on the same Asio worker, which we really should be
	handler_.start(connection_, ident_);
	connection_.start(handler_, ident_);
}

void Client::stop() {
	if(!running_.exchange(false)) {
		return;
	}

	dispatcher_.remove_client(this);
	handler_.stop();
	connection_.stop();
}

bool Client::stopped() const {
	return handler_.stopped() && connection_.stopped();
}

const ClientConnection& Client::connection() const {
	return connection_;
}

const ClientHandler& Client::handler() const {
	return handler_;
}

Client::~Client() {
	stop();
}

} // realm, ember