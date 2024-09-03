/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/PacketHeaders.h>
#include <concepts>
#include <cstddef>

namespace ember::protocol {

// todo, remove INITIAL when codegen is done
enum class State {
	INITIAL, DONE, ERRORED
};

template <typename HeaderT, typename HeaderT::OpcodeType op_, typename Payload>
struct Packet final {
	struct packet_tag_t{};
	using packet_tag = packet_tag_t;

	using OpcodeType = typename HeaderT::OpcodeType;
	using SizeType = typename HeaderT::SizeType;
	using PayloadType = Payload;

	static constexpr OpcodeType opcode = op_;
	static constexpr std::size_t HEADER_WIRE_SIZE = HeaderT::WIRE_SIZE;

	PayloadType payload;

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
struct ServerPacket {
	using Type = Packet<ServerHeader, opcode, Payload>;
};

template<ClientOpcode opcode, typename Payload>
struct ClientPacket {
	using Type = Packet<ClientHeader, opcode, Payload>;
};

template<typename T>
concept is_packet = requires { typename T::packet_tag; };

} // protocol, ember