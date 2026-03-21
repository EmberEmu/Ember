/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/PacketHeaders.h>
#include <protocol/StreamResult.h>
#include <cstddef>

namespace ember::protocol {

template<typename HeaderType, typename HeaderType::OpcodeType op_, typename Payload>
struct Packet final {
	using OpcodeType = typename HeaderType::OpcodeType;
	using SizeType = protocol::SizeType;
	using PayloadType = typename Payload;

	static constexpr OpcodeType opcode = op_;
	static constexpr std::size_t header_wire_size = HeaderType::wire_size;

	Payload payload;

	StreamResult read_payload_from_stream(auto& stream) {
		return payload.read_from_stream(stream);
	}

	StreamResult write_to_stream(auto& stream) const {
		stream << SizeType{} << OpcodeType{};
		return payload.write_to_stream(stream);
	}

	auto operator->() {
		return &payload;
	}

	const auto operator->() const {
		return &payload;
	}
};

template<ServerOpcode opcode, typename Payload>
using ServerPacket = Packet<ServerHeader, opcode, Payload>;

template<ClientOpcode opcode, typename Payload>
using ClientPacket = Packet<ClientHeader, opcode, Payload>;

} // protocol, ember