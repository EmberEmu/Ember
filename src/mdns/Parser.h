/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "DNSDefines.h"
#include <spark/buffers/BinaryStream.h>
#include <shared/smartenum.hpp>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <map>
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

using Labels = std::map<std::uint16_t, std::string>;
using Pointers = std::unordered_map<std::string_view, std::uint16_t>;

// deserialisation
std::string parse_label_notation(spark::BinaryInStream& stream);
void parse_header(Query& query, spark::BinaryInStream& stream);
Question parse_question(Labels& labels, spark::BinaryInStream& stream);
std::vector<std::string> parse_labels(Labels& labels, spark::BinaryInStream& stream);
ResourceRecord parse_resource_record(Labels& labels, spark::BinaryInStream& stream);
void parse_records(Query& query, Labels& labels, spark::BinaryInStream& stream);
Flags decode_flags(std::uint16_t flags);
std::string labels_to_name(std::span<const std::string> labels);

void parse_rdata_a(ResourceRecord& rr, spark::BinaryInStream& stream);
void parse_rdata_txt(ResourceRecord& rr, spark::BinaryInStream& stream);
void parse_rdata_aaaa(ResourceRecord& rr, spark::BinaryInStream& stream);
void parse_rdata_hinfo(ResourceRecord& rr, spark::BinaryInStream& stream);
void parse_rdata_ptr(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata_soa(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata_mx(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata_uri(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata_srv(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);
void parse_rdata_cname(ResourceRecord& rr, detail::Labels& labels, spark::BinaryInStream& stream);

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