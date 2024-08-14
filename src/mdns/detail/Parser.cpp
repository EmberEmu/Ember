/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include <spark/buffers/pmr/BufferAdaptor.h>
#include <logger/Logging.h>
#include <gsl/gsl_util>
#include <boost/endian.hpp>

namespace be = boost::endian;

namespace ember::dns::parser {

Flags decode_flags(const std::uint16_t flags) {
	const Flags parsed{
		.qr = (flags & QR_MASK) >> QR_OFFSET,
		.opcode = static_cast<Opcode>((flags & OPCODE_MASK) >> OPCODE_OFFSET),
		.aa = (flags & AA_MASK) >> AA_OFFSET,
		.tc = (flags & TC_MASK) >> TC_OFFSET,
		.rd = (flags & RD_MASK) >> RD_OFFSET,
		.ra = (flags & RA_MASK) >> RA_OFFSET,
		.z = (flags & Z_MASK) >> Z_OFFSET,
		.ad = (flags & AD_MASK) >> AD_OFFSET,
		.cd = (flags & CD_MASK) >> CD_OFFSET,
		.rcode = static_cast<ReplyCode>((flags & RCODE_MASK) >> RCODE_OFFSET)
	};

	return parsed;
}

std::uint16_t encode_flags(const Flags& flags) {
	std::uint16_t encoded = 0;
	encoded |= flags.qr << QR_OFFSET;
	encoded |= static_cast<std::uint8_t>(flags.opcode) << OPCODE_OFFSET;
	encoded |= flags.aa << AA_OFFSET;
	encoded |= flags.tc << TC_OFFSET;
	encoded |= flags.rd << RD_OFFSET;
	encoded |= flags.ra << RA_OFFSET;
	encoded |= flags.z << Z_OFFSET;
	encoded |= flags.ad << AD_OFFSET;
	encoded |= flags.cd << CD_OFFSET;
	encoded |= static_cast<std::uint8_t>(flags.rcode) << RCODE_OFFSET;
	return encoded;
}

std::string parse_label_notation(std::span<const std::uint8_t> buffer) try {
	spark::io::pmr::BufferReadAdaptor adaptor(buffer);
	spark::io::pmr::BinaryStreamReader stream(adaptor);

	std::stringstream name;
	std::uint8_t length;
	stream >> length;

	if(length) {
		std::string segment;
		stream.get(segment, length);
		name << segment;
	}

	return name.str();
} catch(spark::exception&) {
	throw Result::LABEL_PARSE_ERROR;
}

void parse_header(Query& query, spark::io::pmr::BinaryStreamReader& stream) try {
	stream >> query.header.id;
	std::uint16_t flags;
	stream >> flags;
	stream >> query.header.questions;
	stream >> query.header.answers;
	stream >> query.header.authority_rrs;
	stream >> query.header.additional_rrs;

	be::big_to_native_inplace(query.header.id);
	be::big_to_native_inplace(flags);
	query.header.flags = decode_flags(flags);
	be::big_to_native_inplace(query.header.questions);
	be::big_to_native_inplace(query.header.answers);
	be::big_to_native_inplace(query.header.authority_rrs);
	be::big_to_native_inplace(query.header.additional_rrs);
} catch(spark::exception&) {
	throw Result::HEADER_PARSE_ERROR;
}

Question parse_question(ParseContext& ctx) try {
	Question question;
	question.meta.labels = parse_labels(ctx);
	question.name = labels_to_name(question.meta.labels);
	std::uint16_t type = 0, cc = 0;
	ctx.stream >> type;
	ctx.stream >> cc;
	be::big_to_native_inplace(type);
	be::big_to_native_inplace(cc);

	// handle unicast response flag
	if(cc & UNICAST_RESP_MASK) {
		cc ^= UNICAST_RESP_MASK;
		question.meta.accepts_unicast_response = true;
	}

	question.type = static_cast<RecordType>(type);
	question.cc = static_cast<Class>(cc);
	return question;
} catch(spark::exception&) {
	throw Result::QUESTION_PARSE_ERROR;
}

std::string labels_to_name(std::span<std::string_view> labels) {
	std::stringstream ss;

	for(auto it = labels.begin(); it != labels.end();) {
		ss << *it++;

		if(it != labels.end()) {
			ss << ".";
		}
	}

	return ss.str();
}

std::vector<std::string_view> extract_labels(std::span<const std::uint8_t> buffer, std::size_t offset) {
	std::span chars(reinterpret_cast<const char*>(buffer.data()), buffer.size_bytes());
	std::vector<std::string_view> labels;

	while(true) {
		if(offset >= buffer.size_bytes()) {
			throw Result::BAD_NAME_OFFSET;
		}

		std::uint8_t notation = buffer[offset];

		if(!notation) {
			break;
		}

		notation >>= NOTATION_OFFSET;

		if(notation == NOTATION_STR) {
			std::uint8_t len = buffer[offset];
			labels.emplace_back(chars.data() + offset + 1, len);
			offset += len + 1;
		} else if(notation == NOTATION_PTR) {
			if(offset + 1 >= buffer.size_bytes()) {
				throw Result::BAD_NAME_OFFSET;
			}

			auto ptr = buffer[offset + 1];

			if(ptr >= buffer.size_bytes()) {
				throw Result::BAD_NAME_OFFSET;
			}

			const auto len = buffer[ptr++];

			if(ptr + len >= buffer.size_bytes()) {
				throw Result::BAD_NAME_OFFSET;
			}

			labels.emplace_back(chars.data() + ptr, len);
			offset = ptr + len;
		} else {
			throw Result::BAD_NAME_NOTATION;
		}
	}

	return labels;
}

void skip_stream_labels(spark::io::pmr::BinaryStreamReader& stream) {
	while(true) {
		std::uint8_t notation = 0;
		stream.buffer()->copy(&notation, 1);

		if(notation == 0) {
			stream.skip(1);
			break;
		}

		notation >>= NOTATION_OFFSET;

		if(notation == NOTATION_STR) {
			std::uint8_t len = 0;
			stream >> len;
			stream.skip(len);
		} else if(notation == NOTATION_PTR) {
			stream.skip(2);
			break;
		} else {
			throw Result::BAD_NAME_NOTATION;
		}
	}
}

/*
 * See the comment in write_resource_record() if you want to know what's
 * going on here
 */
std::vector<std::string_view> parse_labels(ParseContext& ctx) try {
	auto labels = extract_labels(ctx.buffer, ctx.stream.total_read());
	skip_stream_labels(ctx.stream);
	return labels;
} catch(spark::exception&) {
	throw Result::NAME_PARSE_ERROR;
}

ResourceRecord parse_resource_record(ParseContext& ctx) try {
	ResourceRecord record;
	auto parsed = parse_labels(ctx);
	record.name = labels_to_name(parsed);
	std::uint16_t type = 0, rc = 0;
	ctx.stream >> type;
	ctx.stream >> rc;
	ctx.stream >> record.ttl;
	ctx.stream >> record.rdata_len;

	be::big_to_native_inplace(type);
	be::big_to_native_inplace(rc);
	be::big_to_native_inplace(record.ttl);
	be::big_to_native_inplace(record.rdata_len);

	// handle unicast response flag
	if(rc & UNICAST_RESP_MASK) {
		rc ^= UNICAST_RESP_MASK; // todo, add metadata for this?
	}

	record.type = static_cast<RecordType>(type);
	record.resource_class = static_cast<Class>(rc);
	parse_rdata(record, ctx);
	return record;
} catch(spark::exception&) {
	throw Result::RR_PARSE_ERROR;
}

void parse_rdata(ResourceRecord& rr, ParseContext& ctx) try {
	switch(rr.type) {
		case RecordType::PTR:
			parse_rdata_ptr(rr, ctx);
			break;
		case RecordType::A:
			parse_rdata_a(rr, ctx);
			break;
		case RecordType::AAAA:
			parse_rdata_aaaa(rr, ctx);
			break;
		case RecordType::SOA:
			parse_rdata_soa(rr, ctx);
			break;
		case RecordType::MX:
			parse_rdata_mx(rr, ctx);
			break;
		case RecordType::TXT:
			parse_rdata_txt(rr, ctx);
			break;
		case RecordType::URI:
			parse_rdata_uri(rr, ctx);
			break;
		case RecordType::SRV:
			parse_rdata_srv(rr, ctx);
			break;
		case RecordType::CNAME:
			parse_rdata_cname(rr, ctx);
			break;
		case RecordType::HINFO:
			parse_rdata_hinfo(rr, ctx);
			break;
		case RecordType::NSEC:
			parse_rdata_nsec(rr, ctx);
			break;
		default:
			throw Result::UNHANDLED_RDATA;
	}
} catch(spark::exception&) {
	throw Result::RR_PARSE_ERROR;
}

void parse_rdata_nsec(ResourceRecord& rr, ParseContext& ctx) {
	Record_NSEC rdata;
	auto labels = parse_labels(ctx);
	rdata.next_domain = labels_to_name(labels);
	std::uint16_t bitmap_len = 0;
	ctx.stream >> bitmap_len;
	be::big_to_native_inplace(bitmap_len);
	ctx.stream.skip(bitmap_len); // don't care about this for now
}

void parse_rdata_hinfo(ResourceRecord& rr, ParseContext& ctx) {
	Record_HINFO rdata;

	// cpu
	std::uint8_t strlen = 0;
	ctx.stream >> strlen;
	rdata.cpu.resize(strlen);
	ctx.stream.get(rdata.cpu.data(), rdata.cpu.size());

	// os
	ctx.stream >> strlen;
	rdata.os.resize(strlen);
	ctx.stream.get(rdata.os.data(), rdata.os.size());

	rr.rdata = rdata;
}

void parse_rdata_a(ResourceRecord& rr, ParseContext& ctx) {
	Record_A rdata;
	ctx.stream >> rdata;
	rr.rdata = rdata;
}

void parse_rdata_aaaa(ResourceRecord& rr, ParseContext& ctx) {
	Record_AAAA rdata;
	ctx.stream >> rdata.ip;
	rr.rdata = rdata;
}

void parse_rdata_cname(ResourceRecord& rr, ParseContext& ctx) {
	Record_CNAME rdata;
	auto labels = parse_labels(ctx);
	rdata.cname = labels_to_name(labels);
	rr.rdata = rdata;
}

void parse_rdata_srv(ResourceRecord& rr, ParseContext& ctx) {
	Record_SRV rdata;
	ctx.stream >> rdata.priority;
	ctx.stream >> rdata.weight;
	ctx.stream >> rdata.port;
	auto labels = parse_labels(ctx);
	rdata.target = labels_to_name(labels);
	be::big_to_native_inplace(rdata.priority);
	be::big_to_native_inplace(rdata.weight);
	be::big_to_native_inplace(rdata.port);
	rr.rdata = rdata;
}

void parse_rdata_uri(ResourceRecord& rr, ParseContext& ctx) {
	Record_URI rdata;
	ctx.stream >> rdata.priority;
	ctx.stream >> rdata.weight;
	auto labels = parse_labels(ctx);
	rdata.target = labels_to_name(labels);
	be::big_to_native_inplace(rdata.priority);
	be::big_to_native_inplace(rdata.weight);
	rr.rdata = rdata;
}

void parse_rdata_ptr(ResourceRecord& rr, ParseContext& ctx) {
	Record_PTR rdata;
	auto labels = parse_labels(ctx);
	rdata.ptrdname = labels_to_name(labels);
	rr.rdata = rdata;
}

void parse_rdata_txt(ResourceRecord& rr, ParseContext& ctx) {
	Record_TXT rdata;
	std::uint16_t remaining = rr.rdata_len;

	while(remaining) {
		std::uint8_t length;
		ctx.stream >> length;
		std::string data;
		data.resize(length);
		ctx.stream.get(data, length);
		rdata.txt.emplace_back(data);
		remaining -= length + 1;
	}

	rr.rdata = rdata;
}

void parse_rdata_mx(ResourceRecord& rr, ParseContext& ctx) {
	Record_MX rdata;
	ctx.stream >> rdata.preference;
	be::big_to_native_inplace(rdata.preference);
	auto labels = parse_labels(ctx);
	rdata.exchange = labels_to_name(labels);
	rr.rdata = rdata;
}

void parse_rdata_soa(ResourceRecord& rr, ParseContext& ctx) {
	Record_SOA rdata;
	auto labels = parse_labels(ctx);
	rdata.mname = labels_to_name(labels);
	labels = parse_labels(ctx);
	rdata.rname = labels_to_name(labels);
	ctx.stream >> rdata.serial;
	ctx.stream >> rdata.refresh;
	ctx.stream >> rdata.retry;
	ctx.stream >> rdata.expire;
	ctx.stream >> rdata.minimum;

	be::big_to_native_inplace(rdata.serial);
	be::big_to_native_inplace(rdata.refresh);
	be::big_to_native_inplace(rdata.retry);
	be::big_to_native_inplace(rdata.expire);
	be::big_to_native_inplace(rdata.minimum);

	rr.rdata = std::move(rdata);
}

void parse_records(Query& query, ParseContext& ctx) {
	for(auto i = 0u; i < query.header.questions; ++i) {
		query.questions.emplace_back(std::move(parse_question(ctx)));
	}

	for(auto i = 0u; i < query.header.answers; ++i) {
		query.answers.emplace_back(std::move(parse_resource_record(ctx)));
	}

	for(auto i = 0u; i < query.header.authority_rrs; ++i) {
		query.authorities.emplace_back(std::move(parse_resource_record(ctx)));
	}

	for(auto i = 0u; i < query.header.additional_rrs; ++i) {
		query.additional.emplace_back(std::move(parse_resource_record(ctx)));
	}
}

void write_header(const Query& query, spark::io::pmr::BinaryStream& stream) {
	stream << be::native_to_big(query.header.id);
	stream << be::native_to_big(encode_flags(query.header.flags));
	stream << be::native_to_big(query.header.questions);
	stream << be::native_to_big(query.header.answers);
	stream << be::native_to_big(query.header.authority_rrs);
	stream << be::native_to_big(query.header.additional_rrs);
}

Pointers write_questions(const Query& query, spark::io::pmr::BinaryStream& stream) {
	Pointers pointers;

	for(auto& question : query.questions) {
		pointers[question.name] = gsl::narrow<std::uint16_t>(stream.total_write());
		write_label_notation(question.name, stream);
		stream << be::native_to_big(static_cast<std::uint16_t>(question.type));
		stream << be::native_to_big(static_cast<std::uint16_t>(question.cc));
	}

	return pointers;
}

std::size_t write_rdata(const ResourceRecord& rr, spark::io::pmr::BinaryStream& stream) {
	const auto write = stream.total_write();

	if(std::holds_alternative<Record_A>(rr.rdata)) {
		const auto& data = std::get<Record_A>(rr.rdata);
		stream << be::native_to_big(data.ip);
	} else if(std::holds_alternative<Record_AAAA>(rr.rdata)) {
		const auto& data = std::get<Record_AAAA>(rr.rdata);
		stream.put(data.ip.data(), data.ip.size());
	} else {
		throw std::runtime_error("Don't know how to serialise this record data");
	}

	return stream.total_write() - write;
}

void write_resource_record(const ResourceRecord& rr, const Pointers& ptrs,
						   spark::io::pmr::BinaryStream& stream) {
	auto it = ptrs.find(rr.name);

	/*
	 * Names in resource records are encoded as either strings
	 * or as pointers to existing strings in the buffer, for
	 * compression purposes. The first two bits specify the
	 * encoding used. If the two leftmost bits are set to 1,
	 * pointer encoding is used. If the two leftmost bits
	 * are set to 0, string encoding is used.
	 *
	 * The remaining bits in pointer notation represent the
	 * offset within the packet that contains the name string.
	 *
	 * The remaining bits in string notation represent the
	 * length of the string segment that follows.
	 *
	 * <00><000000>         = string encoding  ( 8 bits)
	 * <11><00000000000000> = pointer encoding (16 bits)
	 */
	if(it == ptrs.end()) {
		// must be <= 63 bytes (6 bits for length encoding)
		write_label_notation(rr.name, stream);
	} else {
		std::uint16_t ptr = it->second; // should make sure this fits within 30 bits
		ptr ^= (3 << 14);               // set two LSBs to signal pointer encoding
		stream << be::native_to_big(ptr);
	}

	stream << be::native_to_big(static_cast<std::uint16_t>(rr.type));
	stream << be::native_to_big(static_cast<std::uint16_t>(rr.resource_class));
	stream << be::native_to_big(rr.ttl);

	if(!stream.can_write_seek()) {
		// todo
	}

	const auto seek = stream.total_write();
	stream << std::uint16_t(0);
	const auto rdata_len = write_rdata(rr, stream);
	const auto new_seek = stream.total_write();
	stream.write_seek(spark::io::StreamSeek::SK_BUFFER_ABSOLUTE, seek);
	stream << be::native_to_big(gsl::narrow<std::uint16_t>(rdata_len));
	stream.write_seek(spark::io::StreamSeek::SK_BUFFER_ABSOLUTE, new_seek);
}

void write_resource_records(const Query& query, const Pointers& ptrs,
							spark::io::pmr::BinaryStream& stream) {
	for(auto& rr : query.answers) {
		write_resource_record(rr, ptrs, stream);
	}

	for(auto& rr : query.authorities) {
		write_resource_record(rr, ptrs, stream);
	}

	for(auto& rr : query.additional) {
		write_resource_record(rr, ptrs, stream);
	}
}

// todo: replace this monstrosity with a regex
void write_label_notation(std::string_view name, spark::io::pmr::BinaryStream& stream) {
	std::string_view segment(name);
	auto last = 0;

	while(true) {
		auto index = segment.find_first_of('.', last);

		if(index == std::string::npos && segment.size()) {
			segment = segment.substr(1, -1);
			stream << std::uint8_t(segment.size());
			stream.put(segment.data(), segment.size());
			break;
		} else if(index == std::string::npos) {
			break;
		} else {
			const std::string_view print_segment = segment.substr(0, index);
			segment = segment.substr(last ? index : index + 1, -1);
			stream << std::uint8_t(print_segment.size());
			stream.put(print_segment.data(), print_segment.size());
		}

		last = index;
	}

	stream << '\0';
}

} // parser, dns, ember