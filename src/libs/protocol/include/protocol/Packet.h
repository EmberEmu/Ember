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

enum class State {
	INITIAL, CALL_AGAIN, DONE, ERRORED
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

	template<typename reader>
	State read_from_stream(reader& stream) {
		return payload.read_from_stream(stream);
	}

	template<typename writer>
	void write_to_stream(writer& stream) const {
		payload.write_to_stream(stream);
	}

	template<typename writer>
	friend writer& operator<<(writer& stream, const Packet& p) {
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