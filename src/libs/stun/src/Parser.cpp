/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Parser.h>
#include <boost/assert.hpp>

namespace ember::stun {

void Parser::set_logger(LogCB logger, const Verbosity verbosity) {
	logger_ = logger;
	verbosity_ = verbosity;
}

Error Parser::validate_header(const Header& header) {
	if(mode_ == RFCMode::RFC5389 && header.cookie != MAGIC_COOKIE) {
		return Error::RESP_COOKIE_MISSING;
	}

	if(header.length < ATTR_HEADER_LENGTH) {
		return Error::RESP_BAD_HEADER_LENGTH;
	}

	MessageType type{ static_cast<std::uint16_t>(header.type) };

	if(type != MessageType::BINDING_RESPONSE &&
		type != MessageType::BINDING_ERROR_RESPONSE) {
		return Error::RESP_UNHANDLED_RESP_TYPE;
	}

	return Error::OK;
}

attributes::XorMappedAddress
Parser::xor_mapped_address(spark::BinaryInStream& stream, const TxID& id) {
	stream.skip(1); // skip reserved byte
	attributes::XorMappedAddress attr{};
	stream >> attr.family;

	// XOR port with the magic cookie
	stream >> attr.port;
	be::big_to_native_inplace(attr.port);
	attr.port ^= MAGIC_COOKIE >> 16;

	if(attr.family == AddressFamily::IPV4) {
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
		attr.ipv4 ^= MAGIC_COOKIE;
	} else if(attr.family == AddressFamily::IPV6) {
		stream.get(attr.ipv6.begin(), attr.ipv6.end());
		
		for (auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}

		attr.ipv6[0] ^= MAGIC_COOKIE;
		attr.ipv6[1] ^= id.id_5389[0];
		attr.ipv6[2] ^= id.id_5389[1];
		attr.ipv6[3] ^= id.id_5389[2];
	} else {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}

	return attr;
}

attributes::Fingerprint Parser::fingerprint(spark::BinaryInStream& stream) {
	attributes::Fingerprint attr{};
	stream >> attr.crc32;
	be::big_to_native_inplace(attr.crc32);
	return attr;
}

std::size_t Parser::fingerprint_offset() const {
	return fingerprint_offset_;
}

std::size_t Parser::message_integrity_offset() const {
	return msg_integrity_offset_;
}

Header Parser::header_from_stream(spark::BinaryInStream& stream)
{
	Header header{};
	stream >> header.type;
	stream >> header.length;

	if(mode_ == RFCMode::RFC5389) {
		stream >> header.cookie;
		stream.get(header.tx_id.id_5389.begin(), header.tx_id.id_5389.end());
	} else {
		stream.get(header.tx_id.id_3489.begin(), header.tx_id.id_3489.end());
	}

	return header;
}

attributes::Software
Parser::software(spark::BinaryInStream& stream, const std::size_t size) {
	// UTF8 encoded sequence of less than 128 characters (which can be as long as 763 bytes)
	if(size > 763) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_SOFTWARE_ATTR);
	}

	attributes::Software attr{};
	attr.description.resize(size);
	stream.get(attr.description.begin(), attr.description.end());
	return attr;
}

attributes::MessageIntegrity
Parser::message_integrity(spark::BinaryInStream& stream) {
	attributes::MessageIntegrity attr{};
	stream.get(attr.hmac_sha1.begin(), attr.hmac_sha1.end());
	return attr;
}

attributes::MessageIntegrity256
Parser::message_integrity_sha256(spark::BinaryInStream& stream, const std::size_t length) {
	attributes::MessageIntegrity256 attr{};

	if (length < 16 || length > attr.hmac_sha256.size()) {
		throw Error::RESP_BAD_HMAC_SHA_ATTR;
	}

	stream.get(attr.hmac_sha256.begin(), attr.hmac_sha256.begin() + length);
	return attr;
}

attributes::Username
Parser::username(spark::BinaryInStream& stream, const std::size_t size) {
	attributes::Username attr{};
	attr.username.resize(size);
	stream.get(attr.username.begin(), attr.username.end());
	return attr;
}

attributes::ErrorCode
Parser::error_code(spark::BinaryInStream& stream, std::size_t length) {
	attributes::ErrorCode attr{};
	stream >> attr.code;

	if(attr.code & 0xFFE00000) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
	}

	// (╯°□°）╯︵ ┻━┻
	const auto code = ((attr.code >> 8) & 0x07) * 100;
	const auto num = attr.code & 0xFF;

	if(code < 300 || code >= 700) {
		if(mode_ == RFCMode::RFC5389) {
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
		} else if(code < 100) { // original RFC has a wider range (1xx - 6xx)
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
		}
	}

	if(num >= 100) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
	}

	attr.code = code + num;

	std::string reason;
	reason.resize(length - sizeof(attributes::ErrorCode::code));
	stream.get(reason.begin(), reason.end());

	if(reason.size() % 4) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_STRING_BAD_PAD);
	}

	return attr;
}

attributes::UnknownAttributes
Parser::unknown_attributes(spark::BinaryInStream& stream, std::size_t length) {
	if(length % 2) {
		throw Error::RESP_UNK_ATTR_BAD_PAD;
	}
	
	attributes::UnknownAttributes attr{};

	while(length) {
		Attributes attr_type;
		stream >> attr_type;
		be::big_to_native_inplace(attr_type);
		attr.attributes.emplace_back(attr_type);
		length -= sizeof(attr_type);
	}

	if(attr.attributes.size() % 2) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_UNK_ATTR_BAD_PAD);
	}

	return attr;
}

std::vector<attributes::Attribute>
Parser::extract_attributes(spark::BinaryInStream& stream, const TxID& id, const MessageType type) {
	std::vector<attributes::Attribute> attributes;
	bool has_msg_integrity = false;

	while(!stream.empty()) {
		auto attribute = extract_attribute(stream, id, type);

		if(!attribute) {
			continue;
		}

		// if the MESSAGE-INTEGRITY attribute is present, we need to ignore
		// everything that follows except for the FINGERPRINT attribute
		if(has_msg_integrity && !std::get_if<attributes::Fingerprint>(&(*attribute))) {
			fingerprint_offset_ = stream.total_read() - sizeof(attributes::Fingerprint);
			BOOST_ASSERT(msg_integrity_offset_ < stream.total_read());
			break;
		}

		if(!has_msg_integrity && std::get_if<attributes::MessageIntegrity>(&(*attribute))) {
			msg_integrity_offset_ = stream.total_read() - sizeof(attributes::MessageIntegrity);
			BOOST_ASSERT(msg_integrity_offset_ < stream.total_read());
			has_msg_integrity = true;
		}

		attributes.emplace_back(std::move(*attribute));
	}

	return attributes;
}

std::optional<attributes::Attribute> Parser::extract_attribute(spark::BinaryInStream& stream,
                                                               const TxID& id, const MessageType type) {
	Attributes attr_type;
	be::big_uint16_t length;
	stream >> attr_type;
	stream >> length;
	be::big_to_native_inplace(attr_type);

	const bool required = (std::to_underlying(attr_type) >> 15) ^ 1;
	const bool attr_valid = check_attr_validity(attr_type, type, required);

	if(!attr_valid && required) {
		throw Error::RESP_UNEXPECTED_ATTR;
	}

	switch(attr_type) {
		case Attributes::MAPPED_ADDRESS:
			return extract_ip_pair<attributes::MappedAddress>(stream);
		case Attributes::XOR_MAPPED_ADDR_OPT: // it's a faaaaake!
			[[fallthrough]];
		case Attributes::XOR_MAPPED_ADDRESS:
			return xor_mapped_address(stream, id);
		case Attributes::CHANGED_ADDRESS:
			return extract_ipv4_pair<attributes::ChangedAddress>(stream);
		case Attributes::SOURCE_ADDRESS:
			return extract_ipv4_pair<attributes::SourceAddress>(stream);
		case Attributes::OTHER_ADDRESS:
			return extract_ip_pair<attributes::OtherAddress>(stream);
		case Attributes::RESPONSE_ORIGIN:
			return extract_ip_pair<attributes::ResponseOrigin>(stream);
		case Attributes::REFLECTED_FROM:
			return extract_ipv4_pair<attributes::ReflectedFrom>(stream);
		case Attributes::RESPONSE_ADDRESS:
			return extract_ipv4_pair<attributes::ResponseAddress>(stream);
		case Attributes::MESSAGE_INTEGRITY:
			return message_integrity(stream);
		case Attributes::MESSAGE_INTEGRITY_SHA256:
			return message_integrity_sha256(stream, length);
		case Attributes::USERNAME:
			return username(stream, length);
		case Attributes::SOFTWARE:
			return software(stream, length);
		case Attributes::ALTERNATE_SERVER:
			return extract_ip_pair<attributes::AlternateServer>(stream);
		case Attributes::FINGERPRINT:
			return fingerprint(stream);
		case Attributes::ERROR_CODE:
			return error_code(stream, length);
		case Attributes::UNKNOWN_ATTRIBUTES:
			return unknown_attributes(stream, length);
	}

	// todo assert required
	// todo error handling

	stream.skip(length);
	return std::nullopt;
}

bool Parser::check_attr_validity(const Attributes attr_type, const MessageType msg_type,
                                 const bool required) {
	/*
	 * If this attribute is marked as required, we'll look it up in the map
	 * to check whether we know what it is and more importantly whose fault
	 * it is if we can't finish parsing the message, given our current RFC mode
	 */
	if(required) {
		if(const auto rfc = attr_req_lut.find(attr_type); rfc != attr_req_lut.end()) {
			if (!(rfc->second & mode_)) { // definitely not our fault... probably
				logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_REQ_ATTR_SERVER);
				return false;
			}
		}
		else {
			// might be our fault but probably not
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_UNKNOWN_REQ_ATTRIBUTE);
			return false;
		}
	}

	// Check whether this attribute is valid for the given response type
	if(const auto entry = attr_valid_lut.find(attr_type); entry != attr_valid_lut.end()) {
		if(entry->second != msg_type) { // not valid for this type
			logger_(Verbosity::STUN_LOG_DEBUG,
				required? Error::RESP_BAD_REQ_ATTR_SERVER : Error::RESP_UNKNOWN_OPT_ATTRIBUTE);
			return false;
		}
	} else { // not valid for *any* response type
		logger_(Verbosity::STUN_LOG_DEBUG,
			required ? Error::RESP_BAD_REQ_ATTR_SERVER : Error::RESP_UNKNOWN_OPT_ATTRIBUTE);
		return false;
	}

	return true;
}

} // stun, ember