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

Flags Parser::decode_flags(const std::uint16_t flags) {
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

std::uint16_t Parser::encode_flags(const Flags flags) {
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

const Header* Parser::header_overlay(std::span<const std::uint8_t> buffer) {
    return reinterpret_cast<const Header*>(buffer.data());
}

Result Parser::validate(std::span<const std::uint8_t> buffer) {
	if(buffer.size() < DNS_HDR_SIZE) {
		return Result::HEADER_TOO_SMALL;
	}

	if(buffer.size() > MAX_DGRAM_LEN) {
		return Result::PAYLOAD_TOO_LARGE;
	}

    return Result::OK;
}

//// todo: replace this monstrosity with a regex
//void Serialisation::write_label_notation(std::string_view name, BinaryStream& stream) {
//	std::string_view segment(name);
//	auto last = 0;
//
//	while (true) {
//		auto index = segment.find_first_of('.', last);
//
//		if (index == std::string::npos && segment.size()) {
//			segment = segment.substr(1, -1);
//			stream << std::uint8_t(segment.size());
//			stream.put(segment.data(), segment.size());
//			break;
//		}
//		else if (index == std::string::npos) {
//			break;
//		}
//		else {
//			const std::string_view print_segment = segment.substr(0, index);
//			segment = segment.substr(last ? index : index + 1, -1);
//			stream << std::uint8_t(print_segment.size());
//			stream.put(print_segment.data(), print_segment.size());
//		}
//
//		last = index;
//	}
//
//	stream << '\0';
//}
//
//std::string Serialisation::parse_label_notation(BinaryStream& stream) {
//	std::stringstream name;
//	std::uint8_t length;
//	stream >> length;
//
//	while (length) {
//		std::string segment;
//		stream.get(segment, length);
//		name << segment;
//		stream >> length;
//
//		if (length) {
//			name << ".";
//		}
//	}
//
//	return name.str();
//}
//
//void Serialisation::write_header(const Query& query, BinaryStream& stream) {
//	stream << be::native_to_big(query.header.id);
//	stream << be::native_to_big(encode_flags(query.header.flags));
//	stream << be::native_to_big(query.header.questions);
//	stream << be::native_to_big(query.header.answers);
//	stream << be::native_to_big(query.header.authorities);
//	stream << be::native_to_big(query.header.additional);
//}
//
//auto Serialisation::write_questions(const Query& query, BinaryStream& stream) {
//	NamePointers pointers;
//
//	for (auto& question : query.questions) {
//		pointers[question.name] = gsl::narrow<std::uint16_t>(stream.total_write());
//		write_label_notation(question.name, stream);
//		stream << be::native_to_big(static_cast<std::uint16_t>(question.type));
//		stream << be::native_to_big(static_cast<std::uint16_t>(question.rclass));
//	}
//
//	return pointers;
//}
//
//std::size_t Serialisation::write_rdata(const ResourceRecord& rr, BinaryStream& stream) {
//	const auto write = stream.total_write();
//
//	if (std::holds_alternative<Record_A>(rr.rdata)) {
//		const auto& data = std::get<Record_A>(rr.rdata);
//		stream << be::native_to_big(data.ip);
//	}
//	else if (std::holds_alternative<Record_AAAA>(rr.rdata)) {
//		const auto& data = std::get<Record_AAAA>(rr.rdata);
//		stream.put(data.ip.data(), data.ip.size());
//	}
//	else {
//		throw std::runtime_error("Don't know how to serialise this record data");
//	}
//
//	return stream.total_write() - write;
//}
//
//void Serialisation::write_resource_record(const ResourceRecord& rr, BinaryStream& stream,
//	const NamePointers& ptrs) {
//	auto it = ptrs.find(rr.name);
//
//	/*
//	* Names in resource records are encoded as either strings
//	* or as pointers to existing strings in the buffer, for
//	* compression purposes. The first two bits specifies the
//	* encoding used. If the two leftmost bits are set to 1,
//	* pointer encoding is used. If the two leftmost bits
//	* are set to 0, string encoding is used.
//	*
//	* The remaining bits in pointer notation represent the
//	* offset within the packet that contains the name string.
//	*
//	* The remaining bits in string notation represent the
//	* length of the string segment that follows.
//	*
//	* <00><000000>         = string encoding  ( 8 bits)
//	* <11><00000000000000> = pointer encoding (16 bits)
//	*/
//	if (it == ptrs.end()) {
//		// must be <= 63 bytes (6 bits for length encoding)
//		write_label_notation(rr.name, stream);
//	}
//	else {
//		std::uint16_t ptr = it->second; // should make sure this fits within 30 bits
//		ptr ^= (3 << 14); // set two LSBs to signal pointer encoding
//		stream << be::native_to_big(ptr);
//	}
//
//	stream << be::native_to_big(static_cast<std::uint16_t>(rr.type));
//	stream << be::native_to_big(static_cast<std::uint16_t>(rr.resource_class));
//	stream << be::native_to_big(rr.ttl);
//
//	if (stream.can_write_seek()) {
//		const auto seek = stream.total_write();
//		stream << std::uint16_t(0);
//		const auto rdata_len = write_rdata(rr, stream);
//		const auto new_seek = stream.total_write();
//		stream.write_seek(seek, SeekMode::SM_ABSOLUTE);
//		stream << be::native_to_big(gsl::narrow<std::uint16_t>(rdata_len));
//		stream.write_seek(new_seek, SeekMode::SM_ABSOLUTE);
//	}
//	else {
//		NullBuffer null_buff;
//		BinaryStream null_stream(null_buff);
//		const auto rdata_len = write_rdata(rr, null_stream);
//		stream << be::native_to_big(gsl::narrow<std::uint16_t>(rdata_len));
//		write_rdata(rr, stream);
//	}
//}
//
//void Serialisation::write_resource_records(const Query& query, BinaryStream& stream,
//	const NamePointers& ptrs) {
//	for (auto& rr : query.answers) {
//		write_resource_record(rr, stream, ptrs);
//	}
//
//	for (auto& rr : query.authorities) {
//		write_resource_record(rr, stream, ptrs);
//	}
//
//	for (auto& rr : query.additional) {
//		write_resource_record(rr, stream, ptrs);
//	}
//}
//
//void Serialisation::parse_header(Query& query, BinaryStream& stream) {
//	stream >> query.header.id;
//	std::uint16_t flags;
//	stream >> flags;
//	stream >> query.header.questions;
//	stream >> query.header.answers;
//	stream >> query.header.authorities;
//	stream >> query.header.additional;
//
//	be::big_to_native_inplace(query.header.id);
//	be::big_to_native_inplace(flags);
//	query.header.flags = decode_flags(flags);
//	be::big_to_native_inplace(query.header.questions);
//	be::big_to_native_inplace(query.header.answers);
//	be::big_to_native_inplace(query.header.authorities);
//	be::big_to_native_inplace(query.header.additional);
//}
//
//void Serialisation::parse_questions(Query& query, NameMap& names, BinaryStream& stream) {
//	if (!query.header.questions) {
//		return;
//	}
//
//	for (auto i = 0; i < query.header.questions; ++i) {
//		Question question;
//		question.name = parse_name(stream, names);
//		stream >> question.type;
//		stream >> question.rclass;
//
//		be::big_to_native_inplace(reinterpret_cast<std::uint16_t&>(question.type));
//		be::big_to_native_inplace(reinterpret_cast<std::uint16_t&>(question.rclass));
//		query.questions.emplace_back(std::move(question));
//	}
//}
//
///*
// * See the comment in write_resource_record() if you want to know what's
// * going on here
// */
//std::string Serialisation::parse_name(BinaryStream& stream, NameMap& names) {
//	std::uint8_t notation;
//	stream.buffer()->copy(&notation, 1);
//	notation >>= 6;
//
//	if (notation == 0) { // string/label notation
//		const auto name_offset = stream.total_read();
//		const auto name = parse_label_notation(stream);
//		names[name_offset] = name;
//		return name;
//	}
//	else if (notation == 3) { // pointer notation
//		std::uint16_t name_offset;
//		stream >> name_offset;
//		be::big_to_native_inplace(name_offset);
//
//		auto it = names.find(name_offset ^ (3 << 14));
//
//		if (it == names.end()) {
//			throw std::runtime_error("Invalid name offset");
//		}
//
//		return it->second;
//	}
//	else {
//		throw std::runtime_error("Unknown name notation - parse error?");
//	}
//}
//
//ResourceRecord Serialisation::parse_resource_record(BinaryStream& stream, NameMap& names) {
//	ResourceRecord record;
//
//	record.name = parse_name(stream, names);
//	stream >> record.type;
//	stream >> record.resource_class;
//	stream >> record.ttl;
//	stream >> record.rdata_len;
//
//	be::big_to_native_inplace(reinterpret_cast<std::uint16_t&>(record.type));
//	be::big_to_native_inplace(reinterpret_cast<std::uint16_t&>(record.resource_class));
//	be::big_to_native_inplace(record.ttl);
//	be::big_to_native_inplace(record.rdata_len);
//
//	stream.skip(record.rdata_len); // not actually going to parse this
//	return record;
//}
//
//void Serialisation::parse_resource_records(Query& query, NameMap& names, BinaryStream& stream) {
//	for (auto i = 0; i < query.header.answers; ++i) {
//		query.answers.emplace_back(std::move(parse_resource_record(stream, names)));
//	}
//
//	for (auto i = 0; i < query.header.authorities; ++i) {
//		query.authorities.emplace_back(std::move(parse_resource_record(stream, names)));
//	}
//
//	for (auto i = 0; i < query.header.additional; ++i) {
//		query.additional.emplace_back(std::move(parse_resource_record(stream, names)));
//	}
//}
//
//void Serialisation::write(const Query& query, BinaryStream& stream) {
//	write_header(query, stream);
//	const auto ptrs = write_questions(query, stream);
//	write_resource_records(query, stream, ptrs);
//}
//
//Query Serialisation::read(BinaryStream& stream) {
//	Query query;
//	NameMap names;
//	parse_header(query, stream);
//	parse_questions(query, names, stream);
//	parse_resource_records(query, names, stream);
//	return query;
//}


} // dns, ember