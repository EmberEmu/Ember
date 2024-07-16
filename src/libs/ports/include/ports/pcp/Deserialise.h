/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <boost/endian.hpp>
#include <span>
#include <cstdint>

namespace ember::ports {

namespace be = boost::endian;

template<typename T> T deserialise(std::span<const std::uint8_t> buffer){};

template<>
pcp::OptionHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	pcp::OptionHeader message{};
	stream >> message.code;
	stream >> message.reserved;
	stream >> message.length;
	be::big_to_native_inplace(message.length);
	return message;
}

template<>
pcp::MapRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	pcp::MapRequest message{};
	stream >> message.nonce;
	stream >> message.protocol;
	stream >> message.reserved_0;
	stream >> message.internal_port;
	stream >> message.suggested_external_port;
	stream >> message.suggested_external_ip;
	be::big_to_native_inplace(message.internal_port);
	be::big_to_native_inplace(message.suggested_external_port);
	return message;
}

template<>
pcp::RequestHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	pcp::RequestHeader message{};
	stream >> message.version;
	pcp::Opcode opcode{};
	stream >> opcode;
	message.opcode = pcp::Opcode(std::to_underlying(opcode) & 0x7f);
	message.response = std::to_underlying(opcode) >> 7;
	stream >> message.reserved_0;
	stream >> message.lifetime;
	stream >> message.client_ip;
	be::big_to_native_inplace(message.lifetime);
	return message;
}

template<>
pcp::ResponseHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	pcp::ResponseHeader message{};
	stream >> message.version;
	pcp::Opcode opcode{};
	stream >> opcode;
	message.opcode = pcp::Opcode(std::to_underlying(opcode) & 0x7f);
	message.response = std::to_underlying(opcode) >> 7;
	stream >> message.reserved_0;
	stream >> message.result;
	stream >> message.lifetime;
	stream >> message.epoch_time;
	stream >> message.reserved_1;
	be::big_to_native_inplace(message.lifetime);
	be::big_to_native_inplace(message.epoch_time);
	return message;
}

template<>
pcp::MapResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	pcp::MapResponse message{};
	stream >> message.nonce;
	stream >> message.protocol;
	stream >> message.reserved;
	stream >> message.internal_port;
	stream >> message.assigned_external_port;
	stream >> message.assigned_external_ip;
	be::big_to_native_inplace(message.internal_port);
	be::big_to_native_inplace(message.assigned_external_port);
	return message;
}

template<>
natpmp::MapRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	natpmp::MapRequest message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.reserved;
	stream >> message.internal_port;
	stream >> message.external_port;
	stream >> message.lifetime;
	be::big_to_native_inplace(message.internal_port);
	be::big_to_native_inplace(message.external_port);
	be::big_to_native_inplace(message.lifetime);
	return message;
}

template<>
natpmp::MapResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	natpmp::MapResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	stream >> message.internal_port;
	stream >> message.external_port;
	stream >> message.lifetime;
	be::big_to_native_inplace(message.secs_since_epoch);
	be::big_to_native_inplace(message.internal_port);
	be::big_to_native_inplace(message.external_port);
	be::big_to_native_inplace(message.lifetime);
	return message;
}

template<>
natpmp::ExtAddressRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	natpmp::ExtAddressRequest message{};
	stream >> message.version;
	stream >> message.opcode;
	return message;
}

template<>
natpmp::ExtAddressResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	natpmp::ExtAddressResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	stream >> message.external_ip;
	be::big_to_native_inplace(message.result_code);
	be::big_to_native_inplace(message.secs_since_epoch);
	return message;
}

template<>
natpmp::UnsupportedErrorResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	natpmp::UnsupportedErrorResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	be::big_to_native_inplace(message.result_code);
	be::big_to_native_inplace(message.secs_since_epoch);
	return message;
}

} // ports, ember