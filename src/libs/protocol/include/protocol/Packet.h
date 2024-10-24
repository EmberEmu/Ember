/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/PacketHeaders.h>
#include <protocol/Concepts.h>
#include <concepts>
#include <cstddef>

namespace ember::protocol {

// todo, remove INITIAL when codegen is done
enum class State {
	INITIAL, DONE, ERRORED
};

template <typename HeaderType, typename HeaderType::OpcodeType op_, typename Payload>
struct Packet final {
	struct packet_tag_t{};
	using packet_tag = packet_tag_t;

	using OpcodeType = typename HeaderType::OpcodeType;
	using SizeType = typename HeaderType::SizeType;

	static constexpr OpcodeType opcode = op_;
	static constexpr std::size_t HEADER_WIRE_SIZE = HeaderType::WIRE_SIZE;

	Payload payload;

	State read_from_stream(auto& stream) {
		return payload.read_from_stream(stream);
	}

	void write_to_stream(auto& stream) const {
		payload.write_to_stream(stream);
	}

	friend auto& operator<<(auto& stream, const Packet& p) {
		stream << SizeType{} << OpcodeType{};
		p.write_to_stream(stream);
		return stream;
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