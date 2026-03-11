/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <span>
#include <cstdint>

namespace ember::ports {

template<typename T> T deserialise(std::span<const std::uint8_t> buffer){};

template<>
inline pcp::OptionHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	pcp::OptionHeader message{};
	stream >> message.code;
	stream >> message.reserved;
	stream >> message.length;
	return message;
}

template<>
inline pcp::MapRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	pcp::MapRequest message{};
	stream >> message.nonce;
	stream >> message.protocol;
	stream >> message.reserved_0;
	stream >> message.internal_port;
	stream >> message.suggested_external_port;
	stream >> message.suggested_external_ip;
	return message;
}

template<>
inline pcp::RequestHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	pcp::RequestHeader message{};
	stream >> message.version;
	pcp::Opcode opcode{};
	stream >> opcode;
	message.opcode = pcp::Opcode(std::to_underlying(opcode) & 0x7f);
	message.response = std::to_underlying(opcode) >> 7;
	stream >> message.reserved_0;
	stream >> message.lifetime;
	stream >> message.client_ip;
	return message;
}

template<>
inline pcp::ResponseHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

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
	return message;
}

template<>
inline pcp::MapResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	pcp::MapResponse message{};
	stream >> message.nonce;
	stream >> message.protocol;
	stream >> message.reserved;
	stream >> message.internal_port;
	stream >> message.assigned_external_port;
	stream >> message.assigned_external_ip;
	return message;
}

template<>
inline natpmp::MapRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	natpmp::MapRequest message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.reserved;
	stream >> message.internal_port;
	stream >> message.external_port;
	stream >> message.lifetime;
	return message;
}

template<>
inline natpmp::MapResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	natpmp::MapResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	stream >> message.internal_port;
	stream >> message.external_port;
	stream >> message.lifetime;
	return message;
}

template<>
inline natpmp::ExtAddressRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	natpmp::ExtAddressRequest message{};
	stream >> message.version;
	stream >> message.opcode;
	return message;
}

template<>
inline natpmp::ExtAddressResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	natpmp::ExtAddressResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	stream >> message.external_ip;
	return message;
}

template<>
inline natpmp::UnsupportedErrorResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);

	natpmp::UnsupportedErrorResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	return message;
}

} // ports, ember