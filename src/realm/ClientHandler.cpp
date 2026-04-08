/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientLogHelper.h"
#include "Config.h"
#include "ConfigStore.h"
#include "EventDispatcher.h"
#include "packet_log/Helper.h"
#include "states/StateJumpTables.h"
#include <logger/Logger.h>
#include <protocol/Packets.h>
#include <shared/Realm.h>
#include <shared/utility/TickClock.h>
#include <format>
#include <utility>

namespace ember::realm {

void ClientHandler::start() {
	context_.dispatcher.register_handler(this);
	state_update(ClientState::cs_authenticating);
}

void ClientHandler::stop() {
	if(context_.state == ClientState::cs_session_closed) {
		return;
	}

	context_.dispatcher.remove_handler(this);
	state_update(ClientState::cs_session_closed);
}

void ClientHandler::close() {
	connection_->close_session();
}

void ClientHandler::handle_message(StaticBuffer& buffer, const protocol::SizeType msg_size) {
	BinaryStream stream(buffer, msg_size);
	context_.stream = &stream;
	protocol::ClientOpcode opcode;
	stream >> opcode;

	PACKET_TRACE(context_, "-> {}", opcode);
	++packet_counter_;

	// handle ping as a special case
	if(opcode == protocol::ClientOpcode::cmsg_ping) {
		handle_ping(stream);
		return;
	}

	update_packet[context_.state](context_, opcode);
}

bool ClientHandler::handle_self_event(const Event& event) {
	using enum EventType;

	switch(event.type) {
		case kick_self:
			close();
			return true;
		case interval_timer_fire:
			handle_timer();
			return true;
		case packet_log_enable:
			packet_log_start();
			return true;
		case packet_log_disable:
			connection_->packet_log_stop();
			return true;
		default:
			return false;
	}
}

void ClientHandler::handle_event(const Event& event) {
	if(handle_self_event(event)) {
		return;
	}

	update_event[context_.state](context_, event);
}

void ClientHandler::handle_timer() {
	if(!pps_flood_check() || !ping_sent_check()) {
		close();
	}
}

void ClientHandler::state_update(ClientState new_state) {
	CLIENT_DEBUG(context_, "State change, {} => {}",
		ClientState_to_string(context_.state),
		ClientState_to_string(new_state)
	);

	context_.prev_state = context_.state;
	context_.state = new_state;
	exit_state[context_.prev_state](context_);
	enter_state[context_.state](context_);
}

void ClientHandler::skip(BinaryStream& stream) {
	CLIENT_TRACE(context_, "{} skipping message", ClientState_to_string(context_.state));
	stream.skip(stream.read_limit() - stream.total_read());
}

void ClientHandler::handle_ping(BinaryStream& stream) {
	LOG_TRACE(logger_, log_func);

	protocol::cmsg_ping packet;

	if(!deserialise(packet, stream)) {
		return;
	}

	if(!validate_ping(packet.payload)) {
		close();
		return;
	}

	protocol::smsg_pong response;
	response->sequence_id = packet->sequence_id;
	connection_->latency(packet->latency);
	connection_->send(response);
}

void ClientHandler::start_timer(const std::chrono::milliseconds& time) {
	timer_.expires_after(time);
	auto& dispatcher = context_.dispatcher;

	timer_.async_wait([dispatcher, uuid = uuid_](const boost::system::error_code& ec) {
		if(!ec) {
			Event event{ EventType::timer_expired };
			dispatcher.post_event(uuid, event);
		}
	});
}

void ClientHandler::cancel_timer() {
	timer_.cancel_one();
}

/*
 * Ensures the client does not flood the server with a rate of packets that's
 * unlikely to have been generated through normal play. This can be tripped
 * by spamming move set facing packets for too long, which mirrors how the
 * official servers used to behave.
 */
bool ClientHandler::pps_flood_check() {
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

bool ClientHandler::validate_ping(const protocol::client::Ping& ping) {
	if(!ping_sequence_) {
		ping_sequence_ = ping.sequence_id;
		last_tick_ = utility::get_tick_count(utility::as_chrono);
		return true;
	}

	if(!ping.sequence_id) {
		CLIENT_DEBUG(context_, "Zero cmsg_ping sequence");
		return false;
	}

	if(++ping_sequence_ != ping.sequence_id) {
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
		LOG_ERROR(logger_, "Could not remove client logging sink!");
	}

	redirect_sink_.reset();
}

void ClientHandler::packet_log_start() {
	log_redirect_stop(); // avoid generating an infinite packet loop
	const auto& realm = context_.cfg_store.config().realm.name;

	auto plogger = create_packet_logger(
		realm, connection_->remote_address(), uuid_.to_string(), logger_, false
	);

	if(!plogger ) {
		LOG_ERROR(logger_, "Packet logger creation failed!");
		return;
	}

	connection_->packet_log_start(std::move(plogger));
}

/*
 * Helper that decides whether to print the IP address or username
 * and IP address in log outputs, based on whether authentication
 * has completed
 */
std::string_view ClientHandler::client_identify() const {	
	if(context_.client_id) {
		if(client_id_ext_.empty()) {
			client_id_ext_ = std::format(
				"{} ({}, {})", client_id_, context_.client_id->username, context_.client_id->id
			);
		}

		return client_id_ext_;
	}

	if(client_id_.empty()) [[unlikely]] {
		client_id_ = connection_->remote_address();
	}

	return client_id_;
}

ClientHandler::ClientHandler(std::size_t index, executor executor, const ConfigStore& cfg_store,
                             EventDispatcher& dispatcher, RealmQueue& queue, AccountClient& account_rpc,
                             CharacterClient& character_rpc, log::Logger& logger)
	: context_ {
		.timer = ClientTimer(*this),
		.handler = *this,
		.connection = nullptr,
		.cfg_store = cfg_store,
		.dispatcher = dispatcher,
		.queue = queue,
		.account_rpc = account_rpc,
		.character_rpc = character_rpc,
		.logger = logger,
		.state = ClientState::cs_session_closed,
		.prev_state = ClientState::cs_session_closed,
	  }
	, connection_(nullptr)
	, logger_(logger)
	, uuid_(index)
	, timer_(executor)
	, packet_counter_(0)
	, pps_violation_(0)
	, ping_sequence_(0)
	, ping_violation_(0)
	, prev_ping_sequence_(0)
	, timer_events_(0)
	, last_tick_(utility::get_tick_count()) {}

ClientHandler::~ClientHandler() {
	stop();
}

} // realm, ember