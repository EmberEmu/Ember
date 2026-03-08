/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Opcodes.h>
#include <cstdint>

namespace ember::protocol {

using SizeType = std::uint16_t;

struct ServerHeader {
	using OpcodeType = ServerOpcode;
	using SizeType = SizeType;

	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeType);
};

struct ClientHeader {
	using OpcodeType = ClientOpcode;
	using SizeType = SizeType;

	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeType);
};

} // protocol, ember
