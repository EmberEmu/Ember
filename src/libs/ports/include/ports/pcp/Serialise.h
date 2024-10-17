/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Protocol.h>
#include <boost/endian.hpp>
#include <utility>

namespace ember::ports {

namespace be = boost::endian;

void serialise(const pcp::RequestHeader& message, auto& stream) {
	stream << message.version;
	auto opcode = std::to_underlying(message.opcode);
	opcode |= (message.response << 7);
	stream << opcode;
	stream << message.reserved_0;
	stream << be::native_to_big(message.lifetime);
	stream.put(message.client_ip);
}

void serialise(const pcp::MapRequest& message, auto& stream) {
	stream.put(message.nonce);
	stream << message.protocol;
	stream << message.reserved_0;
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.suggested_external_port);
	stream << message.suggested_external_ip;
}

void serialise(const pcp::MapResponse& message, auto& stream) {
	stream << message.nonce;
	stream << message.protocol;
	stream << message.reserved;
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.assigned_external_port);
	stream << message.assigned_external_ip;
}

void serialise(const pcp::ResponseHeader& message, auto& stream) {
	stream << message.version;
	auto opcode = std::to_underlying(message.opcode);
	opcode |= (message.response << 7);
	stream << opcode;
	stream << message.reserved_0;
	stream << message.result;
	stream << be::native_to_big(message.lifetime);
	stream << be::native_to_big(message.epoch_time);
	stream << message.reserved_1;
}

void serialise(const natpmp::MapRequest& message, auto& stream) {
	stream << message.version;
	stream << message.opcode;
	stream << message.reserved;
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.external_port);
	stream << be::native_to_big(message.lifetime);
}

void serialise(const natpmp::MapResponse& message, auto& stream) {
	stream << message.version;
	stream << message.opcode;
	stream << message.result_code;
	stream << be::native_to_big(message.secs_since_epoch);
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.external_port);
	stream << be::native_to_big(message.lifetime);
}

void serialise(const natpmp::ExtAddressRequest& message, auto& stream) {
	stream << message.version;
	stream << message.opcode;
}

void serialise(const natpmp::ExtAddressResponse& message, auto& stream) {
	stream << message.version;
	stream << message.opcode;
	stream << be::native_to_big(message.result_code);
	stream << be::native_to_big(message.secs_since_epoch);
	stream << message.external_ip;
}

void serialise(const natpmp::UnsupportedErrorResponse& message, auto& stream) {
	stream << message.version;
	stream << message.opcode;
	stream << be::native_to_big(message.result_code);
	stream << be::native_to_big(message.secs_since_epoch);
}

void serialise(const pcp::OptionHeader& header, auto& stream) {
	stream << header.code;
	stream << header.reserved;
	stream << be::native_to_big(header.length);
}

} // ports, ember