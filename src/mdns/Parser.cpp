/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include <spark/buffers/SpanBufferAdaptor.h>
#include <logger/Logging.h>
#include <gsl/gsl_util>
#include <boost/endian.hpp>

namespace be = boost::endian;

namespace ember::dns::parser {

std::pair<Result, std::optional<Query>> deserialise(std::span<const std::uint8_t> buffer) try {
	if(buffer.size() > MAX_DGRAM_LEN) {
		return { Result::PAYLOAD_TOO_LARGE, std::nullopt };
	}

	spark::SpanBufferAdaptor adaptor(buffer);
	spark::BinaryInStream stream(adaptor);

	Query query;
	detail::Names names;

	detail::parse_header(query, stream);
	detail::parse_records(query, names, stream);

	if(stream.state() != spark::BinaryInStream::State::OK) {
		return { Result::STREAM_ERROR, std::nullopt };
	}

	return { Result::OK, query };
} catch(Result& r) {
	return { r, std::nullopt };
}

void serialise(const Query& query, spark::BinaryStream& stream) {
	detail::write_header(query, stream);
	const auto ptrs = detail::write_questions(query, stream);
	detail::write_resource_records(query, ptrs, stream);
}

namespace detail {

Flags decode_flags(const std::uint16_t flags) {
	const Flags parsed {
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

std::uint16_t encode_flags(const Flags flags) {
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

std::string parse_label_notation(spark::BinaryInStream& stream) try {
	std::stringstream name;
	std::uint8_t length;
	stream >> length;

	if(length) {
		std::string segment;
		stream.get(segment, length);
		name << segment;
	}

	return name.str();
} catch(spark::buffer_underrun&) {
	throw Result::LABEL_PARSE_ERROR;
}

void parse_header(Query& query, spark::BinaryInStream& stream) try {
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
} catch(spark::buffer_underrun&) {
	throw Result::HEADER_PARSE_ERROR;
}

Question parse_question(detail::Names& names, spark::BinaryInStream& stream) try {
	Question question;
	question.meta.labels = parse_labels(names, stream);
	question.name = labels_to_name(question.meta.labels);
	std::uint16_t type = 0, cc = 0;
	stream >> type;
	stream >> cc;
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
} catch(spark::buffer_underrun&) {
	throw Result::QUESTION_PARSE_ERROR;
}

std::string labels_to_name(const std::vector<std::string>& labels) {
	std::stringstream ss;

	for(auto it = labels.begin(); it != labels.end();) {
		ss << *it++;

		if(it != labels.end()) {
			ss << ".";
		}
	}

	return ss.str();
}

/*
 * See the comment in write_resource_record() if you want to know what's
 * going on here
 */
std::vector<std::string> parse_labels(detail::Names& names, spark::BinaryInStream& stream) try {
	std::vector<std::string> labels;

	while(true) {
		std::uint8_t notation = 0;
		stream.buffer()->copy(&notation, 1);

		if(notation == 0) {
			const auto offset = gsl::narrow_cast<std::uint16_t>(stream.total_read());
			names[offset] = "";
			stream.skip(1);
			break;
		}

		notation >>= NOTATION_OFFSET;

		if(notation == NOTATION_STR) {
			const auto name_offset = gsl::narrow_cast<std::uint16_t>(stream.total_read());
			auto name = parse_label_notation(stream);
			names[name_offset] = name;
			labels.emplace_back(std::move(name));
		} else if(notation == NOTATION_PTR) {
			be::big_uint16_t name_offset;
			stream >> name_offset;

			auto it = names.find(name_offset ^ (3 << 14));

			if(it == names.end()) {
				throw Result::BAD_NAME_OFFSET;
			}
			
			for(auto i = it; it != names.end() && !it->second.empty(); ++it) {
				labels.emplace_back(it->second);
			}

			break;
		} else {
			throw Result::BAD_NAME_NOTATION;
		}
	}

	return labels;
} catch(spark::buffer_underrun&) {
	throw Result::NAME_PARSE_ERROR;
}

ResourceRecord parse_resource_record(detail::Names& names, spark::BinaryInStream& stream) try {
	ResourceRecord record;
	const auto labels = parse_labels(names, stream);
	record.name = labels_to_name(labels);
	std::uint16_t type = 0, rc = 0;
	stream >> type;
	stream >> rc;
	stream >> record.ttl;
	stream >> record.rdata_len;

	be::big_to_native_inplace(type);
	be::big_to_native_inplace(rc);
	be::big_to_native_inplace(record.ttl);
	be::big_to_native_inplace(record.rdata_len);

	// handle unicast response flag
	if (rc & UNICAST_RESP_MASK) {
		rc ^= UNICAST_RESP_MASK; // todo, add metadata for this?
	}

	record.type = static_cast<RecordType>(type);
	record.resource_class = static_cast<Class>(rc);
	parse_rdata(record, names, stream);
	return record;
} catch(spark::buffer_underrun&) {
	throw Result::RR_PARSE_ERROR;
}

void parse_rdata(ResourceRecord& rr, detail::Names& names, spark::BinaryInStream& stream) try {
	switch(rr.type) {
		case RecordType::PTR:
			parse_rdata_ptr(rr, names, stream);
			break;
		case RecordType::A:
			parse_rdata_a(rr, stream);
			break;
		case RecordType::AAAA:
			parse_rdata_aaaa(rr, stream);
			break;
		default:
			throw Result::UNHANDLED_RDATA;
	}
} catch(spark::buffer_underrun&) {
	throw Result::RR_PARSE_ERROR;
}

void parse_rdata_a(ResourceRecord& rr, spark::BinaryInStream& stream) {
	Record_A rdata;
	stream >> rdata;
	rr.rdata = rdata;
}

void parse_rdata_aaaa(ResourceRecord& rr, spark::BinaryInStream& stream) {
	Record_AAAA rdata;
	stream >> rdata.ip;
	rr.rdata = rdata;
}

void parse_rdata_ptr(ResourceRecord& rr, detail::Names& names, spark::BinaryInStream& stream) {
	Record_PTR rdata;
	rdata.ptrdname = labels_to_name(parse_labels(names, stream));
	rr.rdata = rdata;
}

void parse_records(Query& query, detail::Names& names, spark::BinaryInStream& stream) {
	for(auto i = 0u; i < query.header.questions; ++i) {
		query.questions.emplace_back(std::move(parse_question(names, stream)));
	}

	for(auto i = 0u; i < query.header.answers; ++i) {
		query.answers.emplace_back(std::move(parse_resource_record(names, stream)));
	}

	for(auto i = 0u; i < query.header.authority_rrs; ++i) {
		query.authorities.emplace_back(std::move(parse_resource_record(names, stream)));
	}

	for(auto i = 0u; i < query.header.additional_rrs; ++i) {
		query.additional.emplace_back(std::move(parse_resource_record(names, stream)));
	}
}

void write_header(const Query& query, spark::BinaryStream& stream) {
	stream << be::native_to_big(query.header.id);
	stream << be::native_to_big(encode_flags(query.header.flags));
	stream << be::native_to_big(query.header.questions);
	stream << be::native_to_big(query.header.answers);
	stream << be::native_to_big(query.header.authority_rrs);
	stream << be::native_to_big(query.header.additional_rrs);
}

Pointers write_questions(const Query& query, spark::BinaryStream& stream) {
	Pointers pointers;

	for(auto& question : query.questions) {
		pointers[question.name] = gsl::narrow<std::uint16_t>(stream.total_write());
		write_label_notation(question.name, stream);
		stream << be::native_to_big(static_cast<std::uint16_t>(question.type));
		stream << be::native_to_big(static_cast<std::uint16_t>(question.cc));
	}

	return pointers;
}

std::size_t write_rdata(const ResourceRecord& rr, spark::BinaryStream& stream) {
	const auto write = stream.total_write();

	if(std::holds_alternative<Record_A>(rr.rdata)) {
		const auto& data = std::get<Record_A>(rr.rdata);
		stream << be::native_to_big(data.ip);
	}
	else if(std::holds_alternative<Record_AAAA>(rr.rdata)) {
		const auto& data = std::get<Record_AAAA>(rr.rdata);
		stream.put(data.ip.data(), data.ip.size());
	}
	else {
		throw std::runtime_error("Don't know how to serialise this record data");
	}

	return stream.total_write() - write;
}

void write_resource_record(const ResourceRecord& rr, const detail::Pointers& ptrs,
                           spark::BinaryStream& stream) {
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
	stream.write_seek(spark::SeekDir::SD_START, seek);
	stream << be::native_to_big(gsl::narrow<std::uint16_t>(rdata_len));
	stream.write_seek(spark::SeekDir::SD_START, new_seek);
}

void write_resource_records(const Query& query, const detail::Pointers& ptrs,
                            spark::BinaryStream& stream) {
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
void write_label_notation(std::string_view name, spark::BinaryStream& stream) {
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

} // detail

} // parser, dns, ember