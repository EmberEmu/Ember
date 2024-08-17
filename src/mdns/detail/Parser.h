/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../DNSDefines.h"
#include <spark/buffers/pmr/BinaryStream.h>
#include <shared/smartenum.hpp>
#include <expected>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <utility>
#include <cstddef>

namespace ember::dns::parser {

smart_enum_class(Result, std::uint8_t,
	OK, PAYLOAD_TOO_LARGE, NO_QUESTIONS, BAD_NAME_OFFSET,
	BAD_NAME_NOTATION, UNHANDLED_RECORD_TYPE, STREAM_ERROR, NAME_PARSE_ERROR,
	RR_PARSE_ERROR, QUESTION_PARSE_ERROR, HEADER_PARSE_ERROR, LABEL_PARSE_ERROR,
	UNHANDLED_RDATA, STREAM_CANNOT_SEEK
);

using Pointers = std::unordered_map<std::string_view, std::uint16_t>;

struct ParseContext {
	std::span<const std::uint8_t> buffer;
	spark::io::pmr::BinaryStreamReader& stream;
};

// deserialisation
std::string parse_label_notation(std::span<const std::uint8_t> buffer);
void parse_header(Query& query, spark::io::pmr::BinaryStreamReader& stream);
Question parse_question(ParseContext& ctx);
std::vector<std::string_view> parse_labels(ParseContext& ctx);
ResourceRecord parse_resource_record(ParseContext& ctx);
void parse_records(Query& query, ParseContext& ctx);
Flags decode_flags(std::uint16_t flags);
std::string labels_to_name(std::span<std::string_view> labels);

void parse_rdata_a(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_txt(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_aaaa(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_hinfo(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_ptr(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_soa(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_mx(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_uri(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_srv(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_cname(ResourceRecord& rr, ParseContext& ctx);
void parse_rdata_nsec(ResourceRecord& rr, ParseContext& ctx);

// serialisation
void write_header(const Query& query, spark::io::pmr::BinaryStream& stream);
Pointers write_questions(const Query& query, spark::io::pmr::BinaryStream& stream);
std::size_t write_rdata(const ResourceRecord& rr, spark::io::pmr::BinaryStream& stream);
void write_resource_record(const ResourceRecord& rr, const Pointers& ptrs, spark::io::pmr::BinaryStream& stream);
void write_resource_records(const Query& query, const Pointers& ptrs, spark::io::pmr::BinaryStream& stream);
void write_label_notation(std::string_view name, spark::io::pmr::BinaryStream& stream);
std::uint16_t encode_flags(const Flags& flags);

} // parser, dns, ember