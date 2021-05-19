/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "DNSDefines.h"
#include <spark/buffers/BinaryStream.h>
#include <shared/smartenum.hpp>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <optional>
#include <cstddef>

namespace ember::dns {

smart_enum_class(Result, std::uint8_t,
	OK, HEADER_TOO_SMALL, PAYLOAD_TOO_LARGE, NO_QUESTIONS
);

class Parser final {
	using Names = std::unordered_map<std::uint16_t, std::string>;
	using Pointers = std::unordered_map<std::string_view, std::uint16_t>;

	static std::string parse_label_notation(spark::BinaryStream& stream);
	static void parse_header(Query& query, spark::BinaryStream& stream);
	static void parse_questions(Query& query, Names& names, spark::BinaryStream& stream);
	static std::string parse_name(Names& names, spark::BinaryStream& stream);
	static ResourceRecord parse_resource_record(Names& names, spark::BinaryStream& stream);
	static void parse_resource_records(Query& query, Names& names, spark::BinaryStream& stream);

public:
    static Result validate(std::span<const std::uint8_t> buffer);
    static Flags decode_flags(std::uint16_t flags);
	static std::uint16_t encode_flags(Flags flags);
    static const Header* header_overlay(std::span<const std::uint8_t> buffer);
	static std::pair<dns::Result, std::optional<Query>> read(std::span<const std::uint8_t> buffer);
};

} // dns, ember