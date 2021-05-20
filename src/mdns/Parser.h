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

namespace ember::dns::parser {

smart_enum_class(Result, std::uint8_t,
	OK, PAYLOAD_TOO_LARGE, NO_QUESTIONS, BAD_NAME_OFFSET,
	BAD_NAME_NOTATION, UNHANDLED_RECORD_TYPE, STREAM_ERROR, NAME_PARSE_ERROR,
	RR_PARSE_ERROR, QUESTION_PARSE_ERROR, HEADER_PARSE_ERROR, LABEL_PARSE_ERROR,
	UNHANDLED_RDATA
);

namespace detail {

using Names = std::unordered_map<std::uint16_t, std::string>;
using Pointers = std::unordered_map<std::string_view, std::uint16_t>;

// deserialisation
std::string parse_label_notation(spark::BinaryStream& stream);
void parse_header(Query& query, spark::BinaryStream& stream);
void parse_questions(Query& query, Names& names, spark::BinaryStream& stream);
std::string parse_name(Names& names, spark::BinaryStream& stream);
ResourceRecord parse_resource_record(Names& names, spark::BinaryStream& stream);
void parse_resource_records(Query& query, Names& names, spark::BinaryStream& stream);
Flags decode_flags(std::uint16_t flags);
void parse_rdata(ResourceRecord& rr, spark::BinaryStream& stream);

// serialisation
void write_header(const Query& query, spark::BinaryStream& stream);
Pointers write_questions(const Query& query, spark::BinaryStream& stream);
std::size_t write_rdata(const ResourceRecord& rr, spark::BinaryStream& stream);
void write_resource_record(const ResourceRecord& rr, const Pointers& ptrs, spark::BinaryStream& stream);
void write_resource_records(const Query& query, const Pointers& ptrs, spark::BinaryStream& stream);
void write_label_notation(std::string_view name, spark::BinaryStream& stream);
std::uint16_t encode_flags(Flags flags);

} // detail

std::pair<Result, std::optional<Query>> deserialise(std::span<const std::uint8_t> buffer);
void serialise(const Query& query, spark::BinaryStream& stream);

} // parsing, dns, ember