/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Protocol.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <boost/endian.hpp>
#include <span>
#include <cstdint>

namespace ember::ports {

namespace be = boost::endian;

template<typename T>
void serialise(const pcp::RequestHeader& message, spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	auto opcode = std::to_underlying(message.opcode);
	opcode |= (message.response << 7);
	stream << opcode;
	stream << message.reserved_0;
	stream << be::native_to_big(message.lifetime);
	stream.put(message.client_ip);
}

template<typename T>
void serialise(const pcp::MapRequest& message, spark::v2::BinaryStream<T> stream) {
	stream.put(message.nonce);
	stream << message.protocol;
	stream << message.reserved_0;
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.suggested_external_port);
	stream << message.suggested_external_ip;
}

template<typename T>
void serialise(const natpmp::MapRequest& message, spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	stream << message.opcode;
	stream << message.reserved;
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.external_port);
	stream << be::native_to_big(message.lifetime);
}

template<typename T>
void serialise(const natpmp::MapResponse& message, spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	stream << message.opcode;
	stream << message.result_code;
	stream << be::native_to_big(message.secs_since_epoch);
	stream << be::native_to_big(message.internal_port);
	stream << be::native_to_big(message.external_port);
	stream << be::native_to_big(message.lifetime);
}

template<typename T>
void serialise(const natpmp::ExtAddressRequest& message,
               spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	stream << message.opcode;
}

template<typename T>
void serialise(const natpmp::ExtAddressResponse& message,
               spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	stream << message.opcode;
	stream << be::native_to_big(message.result_code);
	stream << be::native_to_big(message.secs_since_epoch);
	stream << message.external_ip;
}

template<typename T>
void serialise(const natpmp::UnsupportedErrorResponse& message,
               spark::v2::BinaryStream<T> stream) {
	stream << message.version;
	stream << message.opcode;
	stream << be::native_to_big(message.result_code);
	stream << be::native_to_big(message.secs_since_epoch);
}

} // ports, ember