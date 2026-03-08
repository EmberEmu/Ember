/*
 * Copyright (c) 2021 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../DNSDefines.h"
#include "../StreamType.h"
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
	ok,
	payload_too_large,
	no_questions,
	bad_name_offset,
	bad_name_notation,
	unhandled_record_type,
	stream_error,
	name_parse_error,
	rr_parse_error,
	question_parse_error,
	header_parse_error,
	label_parse_error,
	unhandled_rdata,
	stream_cannot_seek
);

using Pointers = std::unordered_map<std::string_view, std::uint16_t>;

struct ParseContext {
	std::span<const std::uint8_t> buffer;
	StreamReadBigEndian& stream;
};

// deserialisation
std::string_view parse_label_notation(std::span<std::uint8_t> buffer);
void parse_header(Query& query, StreamReadBigEndian& stream);
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
void write_header(const Query& query, StreamWriteBigEndian& stream);
Pointers write_questions(const Query& query, StreamWriteBigEndian& stream);
std::size_t write_rdata(const ResourceRecord& rr, StreamWriteBigEndian& stream);
void write_resource_record(const ResourceRecord& rr, const Pointers& ptrs, StreamWriteBigEndian& stream);
void write_resource_records(const Query& query, const Pointers& ptrs, StreamWriteBigEndian& stream);
void write_label_notation(const std::string_view name, StreamWriteBigEndian& stream);
std::uint16_t encode_flags(const Flags& flags);

} // parser, dns, ember