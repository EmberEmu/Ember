/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packet.h>
#include <protocol/StreamResult.h>
#include <shared/utility/polyfill/start_lifetime_as>
#include <type_traits>

#include <spark/buffers/NullBuffer.h> // temp

namespace ember::protocol {

/*
 * SmartMessage allows for fast message access by selectively
 * skipping the usual deserialisation step for POD types
 * where all fields are correctly aligned for the target
 * architecture. This relies on the types being generated at
 * the same time as the core.
 * 
 * The provided buffer is presumed to be suitably aligned
 * (buffer % alignof(T) == 0) but runtime validation
 * for every message would negate performance benefits,
 * so we'll fall back to deserialisation for platforms
 * where an unaligned access could cause a crash.
 * 
 * When multiple messages are loaded into a single buffer
 * without padding, such as buffered client streams, it's
 * possible that an occasional unaligned access may occur.
 * Such cases should be profiled to gauge the benefits of
 * using this wrapper.
 * 
 * The API has been designed for compatibility with existing
 * deserialisation functionality in the core, allowing the
 * usual message types to be selectively wrapped.
 * 
 * SmartMessage objects should always be considered to be
 * non-owning, even though this may not always be the case.
 * 
 * We could do this without the stream type template argument
 * but it'd mean having to keep a full packet structure field
 * even when we have no use for it.
 */
template<typename message_type, typename stream_type>
class message_view final {
	using packet_type = typename message_type::PayloadType;

	constexpr static bool allow_unaligned = true; // todo, temp

	consteval static bool use_view() {
		return std::is_same_v<stream_type::contiguous_type, spark::io::is_contiguous>
			/*&& std::is_same_v<typename PacketType::aligned, is_aligned>*/
			&& std::is_standard_layout_v<packet_type>
			&& std::is_trivial_v<packet_type>;
	}

	using packet_view = std::conditional_t<
		use_view(), packet_type*, packet_type
	>;

	packet_view packet;

public:
	using opcode_type = typename message_type::OpcodeType;
	static constexpr opcode_type opcode = message_type::opcode;

	message_view() = default;

	[[nodiscard]] StreamResult write_to_stream(stream_type& stream) {
		return StreamResult::stream_error;
	}

	[[nodiscard]] StreamResult read_from_stream(stream_type& stream) try {
		if constexpr(std::is_pointer_v<packet_view>) {
			packet = std::start_lifetime_as<packet_type>(stream.buffer()->read_ptr());

			/*
			 * This is a temporary testing hack to deal with empty packets and those that have padding
			 * in them, since doing sizeof() is not going to cut it for determing the number of bytes
			 * to skip.
			 *
			 * Although this looks horrible, it's all templated, so the compiler can optimise this gunk
			 * away when the try/catch isn't there (which it is, temporarily) and just replace it with
			 * the number of bytes that would have been written, so we're not paying the cost of deserialising
			 * the packet (this would all be pointless if we were).
			 * 
			 * This will be replaced by packet metadata when they're generated rather than hand-rolled.
			 * It's all just testing, anyway.
			 */
			spark::io::NullBuffer buffer;
			spark::io::BinaryStream null_stream(buffer);
			packet_type dummy;
			dummy.write_to_stream(null_stream);
			const auto written = null_stream.total_write();
			stream.skip(written);
			return stream? StreamResult::success : StreamResult::stream_error;
		} else {
			return packet.read_from_stream(stream);
		}
	} catch(const std::exception& e) {
		return StreamResult::caught_exception;
	}

	StreamResult read_payload_from_stream(stream_type& stream) {
		return read_from_stream(stream);
	}

	auto operator->() {
		if constexpr(std::is_pointer_v<packet_view>) {
			return packet;
		} else {
			return &packet;
		}
	}

	/*
	 * We don't want to allow copying or moving as
	 * the type still contains a potentially unused
	 * message object (expensive). Instead, functions
	 * should accept a reference to the message type,
	 * allowing a SmartMessage<T> to decay to a const T&.
	 */
	message_view& operator=(message_view&) = delete;
	message_view& operator=(message_view&&) = delete;
	message_view(message_view&) = delete;
	message_view(message_view&&) = delete;
};

} // protocol, ember