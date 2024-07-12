/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/pcp/Client.h>
#include <ports/pcp/Deserialise.h>
#include <ports/pcp/Serialise.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <algorithm>
#include <random>

namespace ember::ports {

namespace bai = boost::asio::ip;

Client::Client(const std::string& interface, std::string gateway, boost::asio::io_context& ctx)
	: ctx_(ctx),
	  timer_(ctx),
	  strand_(ctx),
	  transport_(interface, PORT_IN, ctx),
	  gateway_(std::move(gateway)),
	  interface_(interface),
	  has_resolved_(false), resolve_res_(false) {

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
	// active_handler_ should never exist if this handler is called
	// as it's only popped off the stack within the same thread and is
	// either fulfilled or pushed back before handle_message returns
	while(!handlers_.empty()) {
		auto handler = std::move(handlers_.top());
		handlers_.pop();
		handler(std::unexpected(ErrorCode::CONNECTION_FAILURE));
	}

	states_ = {};
	state_ = State::IDLE;
}

ErrorCode Client::handle_pmp_to_pcp_error(std::span<std::uint8_t> buffer) try {
	const auto response = deserialise<natpmp::UnsupportedErrorResponse>(buffer);

	if(response.version != NATPMP_VERSION
	   || response.result_code != natpmp::Result::UNSUPPORTED_VERSION) {
		// we don't understand this message, at all
		return ErrorCode::SERVER_INCOMPATIBLE;
	}

	return ErrorCode::RETRY_NATPMP;
} catch(const spark::exception&) {
	return ErrorCode::SERVER_INCOMPATIBLE;
}

auto Client::parse_mapping_pcp(std::span<std::uint8_t> buffer, MapRequest& result) -> Error {
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
		return ErrorCode::BAD_RESPONSE;
	}

	if(!header.response) {
		return ErrorCode::BAD_RESPONSE;
	}

	if(header.result != pcp::Result::SUCCESS) {
		return { ErrorCode::PCP_CODE, header.result };
	}

	try {
		std::span<const std::uint8_t> body_buff = {
			buffer.begin() + pcp::HEADER_SIZE, buffer.end()
		};

		const auto body = deserialise<pcp::MapResponse>(body_buff);
		
		if(body.nonce != stored_request_.nonce) {
			return ErrorCode::ID_MISMATCH;
		}

		result = MapRequest {
			.internal_port = body.internal_port,
			.external_port = body.assigned_external_port,
			.lifetime = header.lifetime,
			.epoch = header.epoch_time,
			.nonce = body.nonce,
			.external_ip = body.assigned_external_ip,
		};
	} catch(const spark::exception&) {
		return ErrorCode::BAD_RESPONSE;
	}

	return ErrorCode::SUCCESS;
}

void Client::handle_mapping_pcp(std::span<std::uint8_t> buffer) {
	MapRequest result{};
	const auto res = parse_mapping_pcp(buffer, result);
	auto& handler = active_handler_;

	if(res.code == ErrorCode::SUCCESS) {
		handler(result);
	} else if(res.code == ErrorCode::RETRY_NATPMP && !disable_natpmp_) {
		const auto map_res = add_mapping_natpmp(stored_request_);

		if(map_res == ErrorCode::SUCCESS) {
			states_.emplace(State::AWAIT_MAP_RESULT_NATPMP);
			handlers_.emplace(std::move(active_handler_));
		} else {
			handler(std::unexpected(map_res));
		}
	} else {
		handler(std::unexpected(res));
	}
}

void Client::handle_mapping_pmp(std::span<std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint8_t protocol_version{};
	stream >> protocol_version;

	auto& handler = active_handler_;

	if(protocol_version != NATPMP_VERSION) {
		handler(std::unexpected(ErrorCode::SERVER_INCOMPATIBLE));
		return;
	}

	natpmp::MapResponse response{};

	try {
		response = deserialise<natpmp::MapResponse>(buffer);
	} catch(const spark::exception&) {
		handler(std::unexpected(ErrorCode::BAD_RESPONSE));
		return;
	}

	if(response.opcode != natpmp::Opcode::RESP_TCP
	   && response.opcode != natpmp::Opcode::RESP_UDP) {
		handler(std::unexpected(ErrorCode::BAD_RESPONSE));
		return;
	}

	if(response.result_code != natpmp::Result::SUCCESS) {
		handler(std::unexpected(
			Error{ ErrorCode::NATPMP_CODE, response.result_code }
		));
		return;
	}

	const MapRequest result {
		.internal_port = response.internal_port,
		.external_port = response.external_port,
		.lifetime = response.lifetime,
		.epoch = response.secs_since_epoch,
	};

	handler(result);
}

void Client::handle_external_address_pmp(std::span<std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint8_t protocol_version{};
	stream >> protocol_version;

	auto& handler = active_handler_;

	if(protocol_version != NATPMP_VERSION) {
		handler(std::unexpected(ErrorCode::SERVER_INCOMPATIBLE));
		return;
	}

	natpmp::ExtAddressResponse message{};

	try {
		message = deserialise<natpmp::ExtAddressResponse>(buffer);
	} catch(const spark::exception&) {
		handler(std::unexpected(ErrorCode::BAD_RESPONSE));
		return;
	}

	if(message.opcode != natpmp::Opcode::RESP_EXT) {
		handler(std::unexpected(ErrorCode::BAD_RESPONSE));
		return;
	}

	if(message.result_code == natpmp::Result::SUCCESS) {
		const auto v4 = bai::address_v4(message.external_ip);
		const auto v6 = bai::make_address_v6(bai::v4_mapped, v4);

		MapRequest res {
			.external_ip = v6.to_bytes()
		};

		handler(res);
	} else {
		handler(std::unexpected(
			Error { ErrorCode::PCP_CODE, message.result_code })
		);
	}
}

void Client::handle_external_address_pcp(std::span<std::uint8_t> buffer) {
	auto handler = std::move(active_handler_);
	
	MapRequest result{};
	const auto res = parse_mapping_pcp(buffer, result);

	if(res.code == ErrorCode::SUCCESS) {
		handler(result);
	} else if(res.code == ErrorCode::RETRY_NATPMP && !disable_natpmp_) {
		states_.emplace(State::AWAIT_EXTERNAL_ADDRESS_NATPMP);
		handlers_.emplace(std::move(handler));
		get_external_address_pmp();
	} else {
		handler(std::unexpected(res));
	}
}

void Client::finagle_state() {
	active_handler_ = std::move(handlers_.top());
	handlers_.pop();
	state_ = states_.top();
	states_.pop();
}

bool Client::handle_announce(std::span<std::uint8_t> buffer) try {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	std::uint8_t version = 0;
	stream >> version;
	
	if(version == PCP_VERSION) {
		const auto& header = deserialise<pcp::ResponseHeader>(buffer);

		if(header.opcode != pcp::Opcode::ANNOUNCE || header.lifetime != 0) {
			return false;
		}

		std::lock_guard<std::mutex> guard(handler_lock_);
		announce_handler_(header.epoch_time);
		return true;
	} else if(version == NATPMP_VERSION) {
		const auto& message = deserialise<natpmp::ExtAddressResponse>(buffer);

		if(message.opcode != natpmp::Opcode::REQUEST_EXTERNAL) {
			return false;
		}

		std::lock_guard<std::mutex> guard(handler_lock_);
		announce_handler_(message.secs_since_epoch);
		return true;
	}

	return false;
} catch(const spark::exception&) {
	return false;
}

void Client::handle_message(std::span<std::uint8_t> buffer, const bai::udp::endpoint& ep) {
	/*
	 * Upon receiving a response packet, the client MUST check the source IP
     * address, and silently discard the packet if the address is not the
     * address of the gateway to which the request was sent.
	 */
	if(ep.address().to_string() != gateway_ || buffer.empty()) {
		return;
	}
	
	// announce can be unsolicited, so handle as a special-case
	if(handle_announce(buffer)) {
		return;
	}

	while(!states_.empty()) {
		timer_.cancel();
		finagle_state();

		const auto size = states_.size();

		switch(state_) {
			case State::AWAIT_MAP_RESULT_PCP:
				handle_mapping_pcp(buffer);
				break;
			case State::AWAIT_MAP_RESULT_NATPMP:
				handle_mapping_pmp(buffer);
				break;
			case State::AWAIT_EXTERNAL_ADDRESS_NATPMP:
				handle_external_address_pmp(buffer);
				break;
			case State::AWAIT_EXTERNAL_ADDRESS_PCP:
				handle_external_address_pcp(buffer);
				break;
			default:
				break;
				// do nothing
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

ErrorCode Client::add_mapping_natpmp(const MapRequest& request) {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	natpmp::Opcode opcode{};
	
	switch(request.protocol) {
		case Protocol::TCP:
			opcode = natpmp::Opcode::TCP;
			break;
		case Protocol::UDP:
			opcode = natpmp::Opcode::UDP;
			break;
		default:
			return ErrorCode::INVALID_PROTOCOL;
	}

	const natpmp::MapRequest mapping {
		.opcode = opcode,
		.internal_port = request.internal_port,
		.external_port = request.external_port,
		.lifetime = request.lifetime
	};

	try {
		serialise(mapping, stream);
	} catch(const spark::exception&) {
		return ErrorCode::INTERNAL_ERROR;
	}

	send_request(std::move(buffer));
	return ErrorCode::SUCCESS;
}

void Client::timeout_handler() {
	// we'll just error all of the handlers rather than complicate the
	// state machine - should only be one request at any given time
	handlers_.emplace(std::move(active_handler_));

	do {
		auto handler = std::move(handlers_.top());
		handler(std::unexpected(ErrorCode::NO_RESPONSE));
		handlers_.pop();
	} while(!handlers_.empty());

	states_ = {};
	state_ = State::IDLE;
}

void Client::start_retry_timer(const std::chrono::milliseconds& timeout, const int retries) {
	timer_.expires_from_now(timeout);
	timer_.async_wait(strand_.wrap([&, timeout, retries](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		if(retries != MAX_RETRIES) {
			transport_.send(buffer_);
			start_retry_timer(timeout * 2, retries + 1);
		} else {
			finagle_state();
			timeout_handler();
		}
	}));
}

ErrorCode Client::announce_pcp() {
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
		return ErrorCode::INTERNAL_ERROR;
	}

	return ErrorCode::SUCCESS;
}

ErrorCode Client::add_mapping_pcp(const MapRequest& request, bool strict) {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	
	pcp::RequestHeader header {
		.version = PCP_VERSION,
		.opcode = pcp::Opcode::MAP,
		.lifetime = request.lifetime
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

	pcp::Protocol protocol{};

	switch(request.protocol) {
		case Protocol::TCP:
			protocol = pcp::Protocol::TCP;
			break;
		case Protocol::UDP:
			protocol = pcp::Protocol::UDP;
			break;
		case Protocol::ALL:
			protocol = pcp::Protocol::ALL;
			break;
		default:
			return ErrorCode::INVALID_PROTOCOL;
	}

	pcp::MapRequest map {
		.nonce = request.nonce,
		.protocol = protocol,
		.internal_port = request.internal_port,
		.suggested_external_port = request.external_port,
		.suggested_external_ip = request.external_ip
	};

	const auto it = std::find_if(request.nonce.begin(), request.nonce.end(),
		[](const std::uint8_t val) {
			return val != 0;
		});

	if(it == request.nonce.end()) {
		std::random_device engine;
		std::generate(map.nonce.begin(), map.nonce.end(), std::ref(engine));
	}

	try {
		serialise(header, stream);
		serialise(map, stream);
	} catch(const spark::exception&) {
		return ErrorCode::INTERNAL_ERROR;
	}

	if(strict) {
		pcp::OptionHeader option {
			.code = pcp::OptionCode::PREFER_FAILURE
		};

		try {
			serialise(option, stream);
		} catch(const spark::exception&) {
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	stored_request_ = request;
	stored_request_.nonce = map.nonce;
	send_request(std::move(buffer));
	return ErrorCode::SUCCESS;
}

void Client::send_request(std::vector<std::uint8_t> buffer) {
	auto ptr = std::make_shared<std::vector<std::uint8_t>>(std::move(buffer));
	buffer_ = ptr;
	start_retry_timer();
	transport_.send(std::move(ptr));
}

ErrorCode Client::get_external_address_pcp() {
	MapRequest request {
		.protocol = Protocol::UDP,
		.internal_port = 9, // discard protocol
		.external_port = 9,
		.lifetime = 10
	};

	const auto res = add_mapping_pcp(request, false);
	return res;
}

ErrorCode Client::get_external_address_pmp() {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	natpmp::ExtAddressRequest request{};

	try {
		serialise(request, stream);
	} catch(const spark::exception&) {
		return ErrorCode::INTERNAL_ERROR;
	}

	send_request(std::move(buffer));
	return ErrorCode::SUCCESS;
}

std::future<Result> Client::add_mapping(const MapRequest& mapping, bool strict) {
	auto promise = std::make_shared<std::promise<Result>>();
	auto future = promise->get_future();

	add_mapping(mapping, strict, [promise](const Result& result) {
		promise->set_value(result);
	});

	return future;
}

std::future<Result> Client::delete_mapping(const std::uint16_t internal_port,
                                           const Protocol protocol) {
	MapRequest request {
		.protocol = protocol,
		.internal_port = internal_port,
		.external_port = 0,
		.lifetime = 0
	};

	return add_mapping(request, false);
};

std::future<Result> Client::delete_all(const Protocol protocol) {
	auto promise = std::make_shared<std::promise<Result>>();
	auto future = promise->get_future();

	delete_all(protocol, [promise](const Result& result) {
		promise->set_value(result);
	});

	return future;
}

std::future<Result> Client::external_address() {
	auto promise = std::make_shared<std::promise<Result>>();
	auto future = promise->get_future();

	external_address([promise](const Result& result) {
		promise->set_value(result);
	});

	return future;
}

void Client::add_mapping(const MapRequest& mapping, bool strict, RequestHandler handler) {
	has_resolved_.wait(false);

	if(!resolve_res_) {
		handler(std::unexpected(ErrorCode::RESOLVE_FAILURE));
		return;
	}

	const auto res = add_mapping_pcp(mapping, strict);

	if(res == ErrorCode::SUCCESS) {
		states_.push(State::AWAIT_MAP_RESULT_PCP);
		handlers_.emplace(std::move(handler));
	} else {
		handler(std::unexpected(res));
	}
}

void Client::delete_mapping(std::uint16_t internal_port, Protocol protocol,
                            RequestHandler handler) {
	MapRequest request {
		.protocol = protocol,
		.internal_port = internal_port,
		.external_port = 0,
		.lifetime = 0
	};

	add_mapping(request, false, handler);
}

void Client::delete_all(Protocol protocol, RequestHandler handler) {
	has_resolved_.wait(false);

	if(!resolve_res_) {
		handler(std::unexpected(ErrorCode::RESOLVE_FAILURE));
		return;
	}

	const MapRequest request {
		.protocol = protocol,
		.internal_port = 0,
		.external_port = 0,
		.lifetime = 0
	};

	// PCP seems to provide no way to request deletion of all mappings
	// Tested PCP hardware did not accept the same technique as NAT-PMP
	const auto res = add_mapping_natpmp(request);

	if(res == ErrorCode::SUCCESS) {
		states_.emplace(State::AWAIT_MAP_RESULT_NATPMP);
		handlers_.emplace(std::move(handler));
	} else {
		handler(std::unexpected(res));
	}
}

void Client::external_address(RequestHandler handler) {
	has_resolved_.wait(false);

	if(!resolve_res_) {
		handler(std::unexpected(ErrorCode::RESOLVE_FAILURE));
	}

	const auto res = get_external_address_pcp();

	if(res == ErrorCode::SUCCESS) {
		states_.emplace(State::AWAIT_EXTERNAL_ADDRESS_PCP);
		handlers_.emplace(std::move(handler));
	} else {
		handler(std::unexpected(res));
	}
}

void Client::disable_natpmp(const bool disable) {
	disable_natpmp_ = disable;
}

void Client::announce_handler(AnnounceHandler&& handler) {
	if(!handler) {
		throw std::invalid_argument("Callback cannot be null");
	}

	std::lock_guard<std::mutex> guard(handler_lock_);
	announce_handler_ = std::move(handler);
}

} // ports, ember