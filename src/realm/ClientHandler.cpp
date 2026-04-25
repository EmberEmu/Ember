/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientConnection.h"
#include "ClientContextBuilder.h"
#include "ClientLogHelper.h"
#include "ConfigConsts.h"
#include "EventDispatcher.h"
#include "states/StateJumpTables.h"
#include <logger/Logger.h>
#include <protocol/Deserialise.h>
#include <protocol/Packets.h>
#include <protocol/server/Pong.h>
#include <shared/Realm.h>
#include <shared/utility/TickCount.h>
#include <utility>

namespace ember::realm {

void ClientHandler::start(ClientConnection& connection) {
	set_connection(connection);
	state_update(ClientState::cs_authenticating);
}

void ClientHandler::stop() {
	if(state_ == ClientState::cs_session_closed) {
		return;
	}

	state_update(ClientState::cs_session_closed);
}

void ClientHandler::handle_message(BinaryStream& stream) {
	context_.stream(stream);
	protocol::ClientOpcode opcode;
	stream >> opcode;

	PACKET_TRACE(context_, "-> {}", opcode);
	++packet_counter_;

	// handle ping as a special case
	if(opcode == protocol::ClientOpcode::cmsg_ping) {
		handle_ping(stream);
		return;
	}

	update_packet[state_](context_, opcode);

	// if the state didn't handle this message, skip it
	if(!stream.empty()) {
		skip(stream);
	}
}

bool ClientHandler::handle_self_event(const Event& event) {
	using enum EventType;

	switch(event.type) {
		case interval_timer_fire:
			handle_timer();
			return true;
		default:
			return false;
	}
}

void ClientHandler::handle_event(const Event& event) {
	if(handle_self_event(event)) {
		return;
	}

	update_event[state_](context_, event);
}

void ClientHandler::handle_timer() {
	if(!pps_flood_check() || !ping_sent_check()) {
		close_session();
	}
}

void ClientHandler::state_update(ClientState new_state) {
	CLIENT_DEBUG(context_, "State change, {} => {}",
		ClientState_to_string(state_),
		ClientState_to_string(new_state)
	);

	exit_state[state_](context_);
	state_ = new_state;
	enter_state[state_](context_);
}

void ClientHandler::skip(BinaryStream& stream) {
	CLIENT_TRACE(context_, "{} skipping message", ClientState_to_string(state_));
	stream.skip(stream.read_limit() - stream.total_read());
}

void ClientHandler::handle_ping(BinaryStream& stream) {
	LOG_TRACE(logger_, log_func);

	protocol::cmsg_ping packet;

	if(auto result = protocol::deserialise(packet, stream); !result) {
		return stream_err(result);
	}

	if(!validate_ping(packet)) {
		close_session();
		return;
	}

	protocol::smsg_pong response;
	response->sequence_id = packet->sequence_id;
	connection_->latency(packet->latency);
	send(response);
}

// stops the handler and requests for the session to be terminated
void ClientHandler::close_session() {
	Event event { EventType::request_stop };
	context_.dispatcher.post(uuid_, event);
	stop();
}

/*
 * Ensures the client does not flood the server with a rate of packets that's
 * unlikely to have been generated through normal play. This can be tripped
 * by spamming move set facing packets for too long, which mirrors how the
 * official servers used to behave.
 */
bool ClientHandler::pps_flood_check() {
	static_assert(config::broadcast_timer_frequency != 0s);
	const auto packets_per_sec = packet_counter_ / config::broadcast_timer_frequency.count();

	if(packets_per_sec > pps_hard_limit) {
		CLIENT_DEBUG(context_, "Packet rate > hard limit");
		return false;
	}

	if(packets_per_sec > pps_soft_limit) {
		CLIENT_DEBUG(context_, "Packet rate > soft limit");
		++pps_violation_;
	} else if(pps_violation_) {
		--pps_violation_;
	}

	if(pps_violation_ >= pps_grace) {
		CLIENT_DEBUG(context_, "Too many rate limit violations");
		return false;
	}

	packet_counter_ = 0;
	return true;
}

/*
 * Ensures that the client isn't bypassing cmsg_ping validation by simply
 * not sending the packet at all or by sending it once to set the sequence
 * and then never again.
 */
bool ClientHandler::ping_sent_check() {
	// ensure the client has been given enough time since the last timer fired
	const auto& frequency = config::broadcast_timer_frequency;
	const auto expected = ((frequency + ping_leeway) / ping_delta) * timer_events_++;

	if(!expected) {
		return true;
	}

	timer_events_ = 0;

	if(prev_ping_sequence_ == ping_sequence_) {
		CLIENT_DEBUG(context_, "cmsg_ping missed");
		++ping_violation_;
	} else {
		prev_ping_sequence_ = ping_sequence_;
	}

	if(ping_violation_ > ping_grace) {
		CLIENT_DEBUG(context_, "cmsg_pings absent");
		return false;
	}

	return true;
}

bool ClientHandler::validate_ping(const protocol::cmsg_ping& ping) {
	if(!ping_sequence_) {
		ping_sequence_ = ping->sequence_id;
		last_tick_ = utility::get_tick_count(utility::as_chrono);
		return true;
	}

	if(!ping->sequence_id) {
		CLIENT_DEBUG(context_, "Zero cmsg_ping sequence");
		return false;
	}

	if(++ping_sequence_ != ping->sequence_id) {
		CLIENT_DEBUG(context_, "Non-sequential cmsg_ping sequence");
		return false;
	}

	const auto tick = utility::get_tick_count(utility::as_chrono);
	const auto delta = tick - last_tick_;
	last_tick_ = tick;

	if(delta > (ping_delta - ping_leeway) && delta < (ping_delta + ping_leeway)) {
		if(ping_violation_) {
			--ping_violation_;
		}

		return true;
	}

	CLIENT_DEBUG(context_, "cmsg_ping timing violation");

	if(++ping_violation_ > ping_grace) {
		CLIENT_DEBUG(context_, "cmsg_ping violations exceeded grace");
		return false;
	}

	return true;
}

void ClientHandler::log_redirect(LogRedirect::Type type, log::Severity severity) {
	log_redirect_stop(); // remove if we're just changing settings

	auto sink = std::make_shared<ClientSink>(
		context_.dispatcher, uuid_, severity, type, log::Filter(lf_packet_trace)
	);

	logger_.add_sink(sink);
	redirect_sink_ = std::move(sink);
}

void ClientHandler::log_redirect_stop() {
	if(!redirect_sink_) {
		return;
	}

	if(!logger_.remove_sink(redirect_sink_)) {
		LOG_WARN(logger_, "Client logging sink not found, remove failed");
	}

	redirect_sink_.reset();
}

void ClientHandler::stream_err(const protocol::StreamResult& result) {
	CLIENT_DEBUG(context_, "Deserialisation error encountered, {}", result);

	// this is the only one we'll try to recover from
	if(result != protocol::StreamResult::unprocessed_data) {
		close_session();
	}
}

const ClientIdent& ClientHandler::uuid() const {
	return uuid_;
}

std::string_view ClientHandler::whoami() const {	
	return context_.whoami();
}

void ClientHandler::set_connection(ClientConnection& connection) {
	connection_ = &connection;
	context_.connection(connection);
}

bool ClientHandler::stopped() const {
	return state_ == ClientState::cs_session_closed;
}

ClientHandler::ClientHandler(const ClientIdent& ident, ClientContext context, log::Logger& logger)
	: context_(std::move(context))
	, state_(ClientState::cs_session_closed)
	, connection_(nullptr)
	, logger_(logger)
	, uuid_(ident)
	, packet_counter_(0)
	, pps_violation_(0)
	, ping_sequence_(0)
	, ping_violation_(0)
	, prev_ping_sequence_(0)
	, timer_events_(0)
	, last_tick_(utility::get_tick_count()) {
	context_.set_handler(*this);
}

ClientHandler::~ClientHandler() {
	stop();
}

} // realm, ember