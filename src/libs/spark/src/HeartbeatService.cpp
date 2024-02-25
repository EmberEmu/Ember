/*
 * Copyright (c) 2015 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/HeartbeatService.h>
#include <spark/Service.h>
#include <shared/FilterTypes.h>
#include <boost/uuid/uuid_io.hpp>
#include <utility>

namespace ember::spark::inline v1 {

namespace em = ember::messaging;
namespace sc = std::chrono;
using namespace std::placeholders;

HeartbeatService::HeartbeatService(boost::asio::io_context& io_context, const Service* service,
                                   log::Logger* logger) : timer_(io_context),
                                   service_(service), logger_(logger) {
	REGISTER(em::core::Opcode::MSG_PING, em::core::Ping, HeartbeatService::handle_ping);
	REGISTER(em::core::Opcode::MSG_PONG, em::core::Pong, HeartbeatService::handle_pong);
	set_timer();
}

void HeartbeatService::on_message(const Link& link, const Message& message) {
	auto handler = handlers_.find(static_cast<messaging::core::Opcode>(message.opcode));

	if(handler == handlers_.end()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Unhandled message received by core from "
			<< link.description << LOG_ASYNC;
		return;
	}

	if(!handler->second.verify(message)) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Bad message received by core from "
			<< link.description << LOG_ASYNC;
		return;
	}

	handler->second.handle(link, message);
}

void HeartbeatService::on_link_up(const Link& link) {
	std::lock_guard<std::mutex> guard(lock_);
	peers_.emplace_front(link);
}

void HeartbeatService::on_link_down(const Link& link) {
	std::lock_guard<std::mutex> guard(lock_);
	peers_.remove(link);
}

void HeartbeatService::handle_ping(const Link& link, const Message& message) {
	auto ping = flatbuffers::GetRoot<em::core::Ping>(message.data);
	send_pong(link, ping->timestamp());
}

void HeartbeatService::handle_pong(const Link& link, const Message& message) {
	auto pong = flatbuffers::GetRoot<em::core::Pong>(message.data);
	auto time = sc::duration_cast<sc::milliseconds>(sc::steady_clock::now().time_since_epoch()).count();

	if(pong->timestamp()) {
		auto latency = std::chrono::milliseconds(time - pong->timestamp());

		if(latency > LATENCY_WARN_THRESHOLD) {
			LOG_WARN_FILTER(logger_, LF_SPARK)
				<< "[spark] Detected high latency to " << link.description
				<< ":" << boost::uuids::to_string(link.uuid) << LOG_ASYNC;
		}
	}
}

void HeartbeatService::send_ping(const Link& link, std::uint64_t time) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	std::uint16_t opcode = std::to_underlying(messaging::core::Opcode::MSG_PING);
	auto msg = messaging::core::CreatePing(*fbb, time);
	fbb->Finish(msg);
	service_->send(link, opcode, fbb);
}

void HeartbeatService::send_pong(const Link& link, std::uint64_t time) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	std::uint16_t opcode = std::to_underlying(messaging::core::Opcode::MSG_PONG);
	auto msg = messaging::core::CreatePong(*fbb, time);
	fbb->Finish(msg);
	service_->send(link, opcode, fbb);
}

void HeartbeatService::trigger_pings(const boost::system::error_code& ec) {
	if(ec) { // if ec is set, the timer was aborted
		return;
	}

	// generate the time once for all pings
	// not quite as accurate as per-ping but slightly more efficient
	auto time = sc::duration_cast<sc::milliseconds>(
		sc::steady_clock::now().time_since_epoch()).count();

	std::lock_guard<std::mutex> guard(lock_);

	for(auto& link : peers_) {
		send_ping(link, time);
	}

	set_timer();
}

void HeartbeatService::set_timer() {
	timer_.expires_from_now(PING_FREQUENCY);
	timer_.async_wait([&](auto ec) {
		trigger_pings(ec);
	});
}

void HeartbeatService::shutdown() {
	timer_.cancel();
}

} // spark, ember