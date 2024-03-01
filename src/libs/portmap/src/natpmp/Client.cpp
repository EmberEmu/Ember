/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <portmap/natpmp/Client.h>
#include <portmap/natpmp/Deserialise.h>
#include <portmap/natpmp/Serialise.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <algorithm>
#include <random>

namespace ember::portmap::natpmp {

namespace bai = boost::asio::ip;

Client::Client(const std::string& interface, std::string gateway, boost::asio::io_context& ctx)
	: ctx_(ctx), strand_(ctx), gateway_(std::move(gateway)), transport_(interface, ctx),
	  timer_(ctx), interface_(interface), resolve_res_(false), has_resolved_(false) {

	transport_.set_callbacks(
		[&](std::span<std::uint8_t> buffer, const bai::udp::endpoint& ep) {
			ctx_.dispatch(strand_.wrap([&, buffer = std::move(buffer), ep]() {
				handle_message(buffer, ep);
			}));
		},
		[&](const boost::system::error_code&) { 
			ctx_.dispatch(strand_.wrap([&]() {
				handle_connection_error(); 
			}));
		}
	);

	transport_.join_group("224.0.0.1");

	transport_.resolve(gateway_, PORT_OUT,
	                   [&](const boost::system::error_code& ec,
	                       const ba::ip::udp::endpoint& ep) {
		if(!ec) {
			gateway_ = ep.address().to_string();		
		}

		resolve_res_ = !static_cast<bool>(ec);
		has_resolved_ = true;
		has_resolved_.notify_all();
	});
}

void Client::handle_connection_error() {
	// active_promise_ should never exist if this handler is called
	// as it's only popped off the stack within the same thread and is
	// either fulfilled or pushed back before handle_message returns
	while(!promises_.empty()) {
		auto promise = std::move(promises_.top());
		promises_.pop();

		std::visit([&](auto&& arg) {
			arg.set_value(std::unexpected(ErrorType::CONNECTION_FAILURE));
		}, active_promise_);
	}

	states_ = {};
	state_ = State::IDLE;
}

ErrorType Client::handle_pmp_to_pcp_error(std::span<std::uint8_t> buffer) try {
	const auto response = deserialise<UnsupportedErrorResponse>(buffer);

	if(response.version != NATPMP_VERSION
	   || response.opcode != 0
	   || response.result_code != ResultCode::UNSUPPORTED_VERSION) {
		// we don't understand this message, at all
		return ErrorType::SERVER_INCOMPATIBLE;
	}

	return ErrorType::RETRY_NATPMP;
} catch(const spark::exception&) {
	return ErrorType::SERVER_INCOMPATIBLE;
}

auto Client::parse_mapping_pcp(std::span<std::uint8_t> buffer, MappingResult& result) -> Error {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint8_t protocol_version{};
	stream >> protocol_version;

	if(protocol_version != PCP_VERSION) {
		return handle_pmp_to_pcp_error(buffer);
	}

	pcp::ResponseHeader header{};

	try {
		header = deserialise<pcp::ResponseHeader>(buffer);
	} catch(const spark::exception&) {
		return ErrorType::BAD_RESPONSE;
	}

	if(!header.response) {
		return ErrorType::BAD_RESPONSE;
	}

	if(header.result != pcp::Result::SUCCESS) {
		return { ErrorType::PCP_CODE, header.result };
	}

	std::span<const std::uint8_t> body_buff = {
		buffer.begin() + pcp::HEADER_SIZE, buffer.end()
	};

	try {
		const auto body = deserialise<pcp::MapResponse>(body_buff);
		
		result = MappingResult {
			.internal_port = body.internal_port,
			.external_port = body.assigned_external_port,
			.lifetime = header.lifetime,
			.secs_since_epoch = header.epoch_time,
			.external_ip = body.assigned_external_ip
		};
	} catch(const spark::exception&) {
		return ErrorType::BAD_RESPONSE;
	}

	return ErrorType::SUCCESS;
}

void Client::handle_mapping_pcp(std::span<std::uint8_t> buffer) {
	MappingResult result{};
	const auto res = parse_mapping_pcp(buffer, result);
	auto& promise = std::get<std::promise<MapResult>>(active_promise_);

	if(res.type == ErrorType::SUCCESS) {
		promise.set_value(result);
	} else if(res.type == ErrorType::RETRY_NATPMP) {
		const auto map_res = add_mapping_natpmp(stored_request_);

		if(map_res == ErrorType::SUCCESS) {
			states_.emplace(State::AWAITING_MAPPING_RESULT_PMP);
			promises_.emplace(std::move(active_promise_));
		} else {
			promise.set_value(std::unexpected(map_res));
		}
	} else {
		promise.set_value(std::unexpected(res));
	}
}

void Client::handle_mapping_pmp(std::span<std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint8_t protocol_version{};
	stream >> protocol_version;

	auto& promise = std::get<std::promise<MapResult>>(active_promise_);

	if(protocol_version != NATPMP_VERSION) {
		promise.set_value(std::unexpected(ErrorType::SERVER_INCOMPATIBLE));
		return;
	}

	MappingResponse response{};

	try {
		response = deserialise<MappingResponse>(buffer);
	} catch(const spark::exception&) {
		promise.set_value(std::unexpected(ErrorType::BAD_RESPONSE));
		return;
	}

	if(response.result_code != ResultCode::SUCCESS) {
		promise.set_value(std::unexpected(
			Error{ ErrorType::NATPMP_CODE, response.result_code }
		));
		return;
	}

	const MappingResult result {
		.internal_port = response.internal_port,
		.external_port = response.external_port,
		.lifetime = response.lifetime,
		.secs_since_epoch = response.secs_since_epoch,
	};

	promise.set_value(result);
}

void Client::handle_external_address_pmp(std::span<std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint8_t protocol_version{};
	stream >> protocol_version;

	auto& promise = std::get<std::promise<ExternalAddress>>(active_promise_);

	if(protocol_version != NATPMP_VERSION) {
		promise.set_value(std::unexpected(ErrorType::SERVER_INCOMPATIBLE));
		return;
	}

	ExtAddressResponse message{};

	try {
		message = deserialise<ExtAddressResponse>(buffer);
	} catch(const spark::exception&) {
		promise.set_value(std::unexpected(ErrorType::BAD_RESPONSE));
		return;
	}

	if(message.opcode != NATPMP_RESULT) {
		promise.set_value(std::unexpected(ErrorType::BAD_RESPONSE));
		return;
	}

	if(message.result_code == ResultCode::SUCCESS) {
		const auto v4 = bai::address_v4(message.external_ip);
		const auto v6 = bai::make_address_v6(bai::v4_mapped, v4);
		promise.set_value(v6.to_bytes());
	} else {
		promise.set_value(std::unexpected(
			Error { ErrorType::PCP_CODE, message.result_code })
		);
	}
}

void Client::handle_external_address_pcp(std::span<std::uint8_t> buffer) {
	auto promise = std::move(std::get<std::promise<ExternalAddress>>(active_promise_));
	finagle_state(); // fast-forward to the next state
	
	MappingResult result{};
	const auto res = parse_mapping_pcp(buffer, result);

	if(res.type == ErrorType::SUCCESS) {
		promise.set_value(result.external_ip);
	} else if(res.type == ErrorType::RETRY_NATPMP) {
		states_.emplace(State::AWAITING_EXTERNAL_ADDRESS_PMP);
		promises_.emplace(std::move(promise));
		get_external_address_pmp();
	} else {
		promise.set_value(std::unexpected(res));
	}
}

void Client::finagle_state() {
	active_promise_ = std::move(promises_.top());
	promises_.pop();
	state_ = states_.top();
	states_.pop();
}

void Client::handle_message(std::span<std::uint8_t> buffer, const bai::udp::endpoint& ep) {
	/*
	 * Upon receiving a response packet, the client MUST check the source IP
     * address, and silently discard the packet if the address is not the
     * address of the gateway to which the request was sent.
	 */
	if(ep.address().to_string() != gateway_) {
		return;
	}

	if(buffer.empty()) {
		return;
	}
	while(!states_.empty()) {
		finagle_state();
		const auto size = states_.size();

		switch(state_) {
			case State::AWAITING_MAPPING_RESULT_PCP:
				timer_.cancel();
				handle_mapping_pcp(buffer);
				break;
			case State::AWAITING_MAPPING_RESULT_PMP:
				timer_.cancel();
				handle_mapping_pmp(buffer);
				break;
			case State::AWAITING_EXTERNAL_ADDRESS_PMP:
				timer_.cancel();
				handle_external_address_pmp(buffer);
				break;
			case State::AWAITING_EXTERNAL_ADDRESS_PCP:
				timer_.cancel();
				handle_external_address_pcp(buffer);
				break;
		}
		
		// a handler has pushed a new state, let it do the work
		if(states_.size() != size) {
			break;
		}
	}

	if(states_.empty()) {
		state_ = State::IDLE;
	}
}

ErrorType Client::add_mapping_natpmp(const RequestMapping& mapping) {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	try {
		serialise(mapping, stream);
	} catch(const spark::exception&) {
		return ErrorType::INTERNAL_ERROR;
	}

	send_request(std::move(buffer));
	return ErrorType::SUCCESS;
}

void Client::timeout_promise() {
	// we'll just error all of the promises rather than complicate the
	// state machine - should only be one request at any given time
	promises_.emplace(std::move(active_promise_));

	do {
		active_promise_ = std::move(promises_.top());
		promises_.pop();

		std::visit([&](auto&& arg) {
			arg.set_value(std::unexpected(ErrorType::NO_RESPONSE));
		}, active_promise_);
	} while(!promises_.empty());

	states_ = {};
	state_ = State::IDLE;
}

void Client::start_retry_timer(const std::chrono::milliseconds timeout, int retries) {
	timer_.expires_from_now(timeout);
	timer_.async_wait(strand_.wrap([&, timeout, retries](const boost::system::error_code& ec) mutable {
		if(ec) {
			return;
		}

		if(retries) {
			transport_.send(last_buffer_);
			start_retry_timer(timeout * 2, --retries);
		} else {
			finagle_state();
			timeout_promise();
		}
	}));
}

ErrorType Client::announce_pcp() {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	
	pcp::RequestHeader header {
		.version = PCP_VERSION,
		.opcode = pcp::Opcode::ANNOUNCE,
		.lifetime = 0
	};

	const auto address = bai::make_address(interface_);
	bai::address_v6 v6{};

	if(address.is_v6()) {
		v6 = address.to_v6();
	} else {
		v6 = bai::make_address_v6(bai::v4_mapped, address.to_v4());
	}

	const auto& bytes = v6.to_bytes();
	std::copy(bytes.begin(), bytes.end(), header.client_ip.begin());

	try {
		serialise(header, stream);
		transport_.send(std::move(buffer));
	} catch(const spark::exception&) {
		return ErrorType::INTERNAL_ERROR;
	}

	return ErrorType::SUCCESS;
}

ErrorType Client::add_mapping_pcp(const RequestMapping& mapping) {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	
	pcp::RequestHeader header {
		.version = PCP_VERSION,
		.opcode = pcp::Opcode::MAP,
		.lifetime = mapping.lifetime
	};

	const auto address = bai::make_address(interface_);
	bai::address_v6 v6{};

	if(address.is_v6()) {
		v6 = address.to_v6();
	} else {
		v6 = bai::make_address_v6(bai::v4_mapped, address.to_v4());
	}

	const auto& bytes = v6.to_bytes();
	std::copy(bytes.begin(), bytes.end(), header.client_ip.begin());

	const auto protocol = (mapping.opcode == Protocol::MAP_TCP)?
		pcp::Protocol::TCP : pcp::Protocol::UDP;

	pcp::MapRequest map {
		.protocol = protocol,
		.internal_port = mapping.internal_port,
		.suggested_external_port = mapping.external_port
	};

	const auto it = std::find_if(mapping.nonce.begin(), mapping.nonce.end(),
		[](const std::uint8_t val) {
			return val != 0;
		});

	if(it == mapping.nonce.end()) {
;		std::random_device engine;
		std::generate(map.nonce.begin(), map.nonce.end(), std::ref(engine));
	}

	try {
		serialise(header, stream);
		serialise(map, stream);
	} catch(const spark::exception&) {
		return ErrorType::INTERNAL_ERROR;
	}

	send_request(std::move(buffer));
	return ErrorType::SUCCESS;
}

void Client::send_request(std::vector<std::uint8_t> buffer) {
	auto ptr = std::make_shared<std::vector<std::uint8_t>>(std::move(buffer));
	last_buffer_ = ptr;
	start_retry_timer();
	transport_.send(std::move(ptr));
}

ErrorType Client::get_external_address_pcp() {
	std::promise<MapResult> internal_promise;

	RequestMapping request {
		.opcode = Protocol::MAP_UDP,
		.internal_port = 9, // discard protocol
		.external_port = 9,
		.lifetime = 10
	};

	const auto res = add_mapping_pcp(request);

	if(res == ErrorType::SUCCESS) {
		states_.emplace(State::AWAITING_MAPPING_RESULT_PCP);
		promises_.emplace(std::move(internal_promise));
	}

	return res;
}

ErrorType Client::get_external_address_pmp() {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	RequestExtAddress request{};

	try {
		serialise(request, stream);
	} catch(const spark::exception&) {
		return ErrorType::INTERNAL_ERROR;
	}

	send_request(std::move(buffer));
	return ErrorType::SUCCESS;
}

std::future<Client::MapResult> Client::add_mapping(RequestMapping mapping) {
	has_resolved_.wait(false);

	std::promise<MapResult> promise;
	auto future = promise.get_future();

	if(!resolve_res_) {
		promise.set_value(std::unexpected(ErrorType::RESOLVE_FAILURE));
		return future;
	}

	const auto res = add_mapping_pcp(mapping);

	if(res == ErrorType::SUCCESS) {
		states_.push(State::AWAITING_MAPPING_RESULT_PCP);
		promises_.emplace(std::move(promise));
	} else {
		promise.set_value(std::unexpected(res));
	}

	return future;
}

std::future<Client::MapResult> Client::delete_mapping(const std::uint16_t internal_port,
                                                      const Protocol protocol) {
	RequestMapping request {
		.opcode = protocol,
		.internal_port = internal_port,
		.external_port = 0,
		.lifetime = 0
	};

	return add_mapping(request);
};

std::future<Client::MapResult> Client::delete_all(const Protocol protocol) {
	has_resolved_.wait(false);

	std::promise<MapResult> promise;
	auto future = promise.get_future();

	if(!resolve_res_) {
		promise.set_value(std::unexpected(ErrorType::RESOLVE_FAILURE));
		return future;
	}

	const RequestMapping request {
		.opcode = protocol,
		.internal_port = 0,
		.external_port = 0,
		.lifetime = 0
	};

	// PCP seems to provide no way to request deletion of all mappings
	// Tested PCP hardware did not accept the same technique as NAT-PMP
	const auto res = add_mapping_natpmp(request);

	if(res == ErrorType::SUCCESS) {
		states_.emplace(State::AWAITING_MAPPING_RESULT_PMP);
		promises_.emplace(std::move(promise));
	} else {
		promise.set_value(std::unexpected(res));
	}

	return future;
}

std::future<Client::ExternalAddress> Client::external_address() {
	has_resolved_.wait(false);

	std::promise<ExternalAddress> promise;
	auto future = promise.get_future();

	if(!resolve_res_) {
		promise.set_value(std::unexpected(ErrorType::RESOLVE_FAILURE));
	}

	const auto res = get_external_address_pcp();

	if(res == ErrorType::SUCCESS) {
		states_.emplace(State::AWAITING_EXTERNAL_ADDRESS_PCP);
		promises_.emplace(std::move(promise));
	} else {
		promise.set_value(std::unexpected(res));
	}

	return future;
}

} // natpmp, portmap, ember