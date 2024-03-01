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

template<typename T> T deserialise(std::span<const std::uint8_t> buffer){};

template<>
pcp::ResponseHeader deserialise(std::span<const std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	pcp::ResponseHeader message{};
	stream >> message.version;
	pcp::Opcode opcode{};
	stream >> opcode;
	message.opcode = pcp::Opcode(std::to_underlying(opcode) & 0x7f);
	message.response = std::to_underlying(opcode) & (0x80 >> 7);
	stream >> message.reserved_0;
	stream >> message.result;
	stream >> message.lifetime;
	stream.get(message.reserved_1.begin(), message.reserved_1.end());
	be::big_to_native_inplace(message.lifetime);
	be::big_to_native_inplace(message.epoch_time);
	return message;
}

template<>
pcp::MapResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	pcp::MapResponse message{};
	stream.get(message.nonce.begin(), message.nonce.end());
	stream >> message.protocol;
	stream.get(message.reserved.begin(), message.reserved.end());
	stream >> message.internal_port;
	stream >> message.assigned_external_port;
	stream.get(message.assigned_external_ip.begin(), message.assigned_external_ip.end());
	be::big_to_native_inplace(message.internal_port);
	be::big_to_native_inplace(message.assigned_external_port);
	return message;
}

template<>
natpmp::MapRequest deserialise(std::span<const std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

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
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

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
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	natpmp::ExtAddressRequest message{};
	stream >> message.version;
	stream >> message.opcode;
	return message;
}

template<>
natpmp::ExtAddressResponse deserialise(std::span<const std::uint8_t> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

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
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);

	natpmp::UnsupportedErrorResponse message{};
	stream >> message.version;
	stream >> message.opcode;
	stream >> message.result_code;
	stream >> message.secs_since_epoch;
	be::big_to_native_inplace(message.result_code);
	be::big_to_native_inplace(message.secs_since_epoch);
	return message;
}

} // natpmp, ports, ember