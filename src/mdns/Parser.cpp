/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include <logger/Logging.h>

namespace ember::dns {

Flags Parser::extract_flags(const Header& header) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	// todo, fix casting mess
	const Flags flags {
		.qr = (uint16_t)((header.flags & QR_MASK) >> QR_OFFSET),
		.opcode = (uint16_t)((header.flags & OPCODE_MASK) >> OPCODE_OFFSET),
		.aa = (uint16_t)((header.flags & AA_MASK) >> AA_OFFSET),
		.tc = (uint16_t)((header.flags & TC_MASK) >> TC_OFFSET),
		.rd = (uint16_t)((header.flags & RD_MASK) >> RD_OFFSET),
		.ra = (uint16_t)((header.flags & RA_MASK) >> RA_OFFSET),
		.z = (uint16_t)((header.flags & Z_MASK) >> Z_OFFSET),
		.rcode = (uint16_t)((header.flags & RCODE_MASK) >> RCODE_OFFSET)
	};

    return flags;
}

const Header* Parser::header_overlay(std::span<const std::byte> buffer) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;
    return reinterpret_cast<const Header*>(buffer.data());
}

Result Parser::validate(std::span<const std::byte> buffer) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;
    return Result::VALIDATE_OK;
}


} // dns, ember