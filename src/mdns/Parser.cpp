/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"

namespace ember::dns {

Flags Parser::extract_flags(const Header& header) {
    Flags flags;
    flags.qr = (header.flags & QR_MASK) >> QR_OFFSET;
    flags.opcode = (header.flags & OPCODE_MASK) >> OPCODE_OFFSET;
    flags.aa = (header.flags & AA_MASK) >> AA_OFFSET;
    flags.tc = (header.flags & TC_MASK) >> TC_OFFSET;
    flags.rd = (header.flags & RD_MASK) >> RD_OFFSET;
    flags.ra = (header.flags & RA_MASK) >> RA_OFFSET;
    flags.z = (header.flags & Z_MASK) >> Z_OFFSET;
    flags.rcode = (header.flags & RCODE_MASK) >> RCODE_OFFSET;
    return flags;
}

const Header* Parser::header_overlay(std::span<const std::byte> buffer) {
    return reinterpret_cast<const Header*>(buffer.data());
}

Result Parser::validate(std::span<const std::byte> buffer) {
    return Result::VALIDATE_OK;
}

} // dns, ember