/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Client.h"
#include "EventDispatcher.h"
#include "packet_log/Helper.h"
#include <logger/Logger.h>
#include <cassert>

using namespace std::string_view_literals;

namespace ember::realm {

Client::Client(tcp_socket socket, std::size_t index, EventDispatcher& dispatcher, log::Logger& logger,
               const ClientHandlerBuilder& ch_builder, const ClientConnectionBuilder& cc_builder)
	: ident_(dispatcher.register_client(this, index))
	, handler_(ch_builder.create(ident_, socket.get_executor()))
	, connection_(cc_builder.create(std::move(socket), ident_))
	, dispatcher_(dispatcher)
	, logger_(logger)
	, running_(false) {}

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
		case request_stop_connection:
			[[fallthrough]];
		case request_stop_handler:
			handle_request_stop(event.type);
			return true;
		default:
			return false;
	}
}

void Client::handle_kick() {
	assert(close_fn_);
	LOG_DEBUG(logger_, "{}, kicking self", handler_.client_identify());
	stop();
	close_fn_();
}

void Client::handle_request_stop(EventType type) {
	assert(close_fn_);
	LOG_DEBUG(logger_, "{}, {} requested stop",
		handler_.client_identify(), stop_component_name(type));
	stop();
	close_fn_();
}

std::string_view Client::stop_component_name(EventType type) const {
	using enum EventType;

	switch(type) {
		case request_stop_connection:
			return "connection";
		case request_stop_handler:
			return "handler";
		default:
			return "unknown";
	}
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

	if(running_) {
		handler_.handle_event(event);
	}
}

void Client::start() {
	assert(!running_);
	// Referencing each other like this is fine because they won't start running until we return
	// - that's assuming we're running on the same Asio worker, which we really should be
	handler_.start(connection_);
	connection_.start(handler_);
	running_ = true;
}

void Client::stop() {
	if(!running_) {
		return;
	}

	dispatcher_.remove_client(this);
	handler_.stop();
	connection_.stop();
	running_ = false;
}

void Client::on_close(OnClose on_close) {
	assert(on_close);
	close_fn_ = on_close;
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