/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "StreamTypes.h"
#include "Opcodes.h"
#include <concepts>

namespace ember::grunt {

struct Packet {
	enum class State {
		initial,
		call_again,
		done,
		err_stream_err,
		err_username_too_long,
		err_decompress_err,
		err_compress_err,
		err_survey_too_big,
		err_invalid_generator_length
	};

	Opcode opcode;

	explicit Packet(Opcode opcode) : opcode(opcode) { }

	virtual State read_from_stream(PacketStream& stream) = 0;
	virtual State write_to_stream(PacketStream& stream) const = 0;
	virtual ~Packet() = default;

	template<std::derived_from<Packet> _ty>
	auto& as() {
		return static_cast<_ty&>(*this);
	}

	template<std::derived_from<Packet> _ty>
	auto& as() const {
		return static_cast<const _ty&>(*this);
	}
};

} // client, grunt, ember