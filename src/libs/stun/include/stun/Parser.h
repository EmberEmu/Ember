/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Attributes.h>
#include <stun/Transaction.h>
#include <stun/Logging.h>
#include <stun/Protocol.h>
#include <spark/buffers/BinaryInStream.h>
#include <boost/endian.hpp>
#include <cstddef>
#include <optional>
#include <vector>

namespace ember::stun {

namespace be = boost::endian;

class Parser {
	RFCMode mode_;
	LogCB logger_ = [](Verbosity, Error) {};
	Verbosity verbosity_ = Verbosity::STUN_LOG_TRIVIAL;

public:
	Parser(RFCMode mode) : mode_(mode) {}

	void set_logger(LogCB logger, const Verbosity verbosity) { logger_ = logger; verbosity_ = verbosity; } // todo, move
	Error validate_header(const Header& header);
	Header header_from_stream(spark::BinaryInStream& stream);
	bool check_attr_validity(Attributes attr_type, MessageType msg_type, bool required);

	// individual attributes
	template<typename T> auto extract_ip_pair(spark::BinaryInStream& stream);
	template<typename T> auto extract_ipv4_pair(spark::BinaryInStream& stream);
	attributes::XorMappedAddress xor_mapped_address(spark::BinaryInStream& stream, const TxID& id);
	attributes::UnknownAttributes unknown_attributes(spark::BinaryInStream& stream, std::size_t length);
	attributes::ErrorCode error_code(spark::BinaryInStream& stream, std::size_t length);
	attributes::MessageIntegrity message_integrity(spark::BinaryInStream& stream);
	attributes::MessageIntegrity256 message_integrity_sha256(spark::BinaryInStream& stream, std::size_t length);
	attributes::Username username(spark::BinaryInStream& stream, std::size_t size);
	attributes::Software software(spark::BinaryInStream& stream, std::size_t size);
	attributes::Fingerprint fingerprint(spark::BinaryInStream& stream);

	// extract all attributes from a stream
	std::vector<attributes::Attribute>
	extract_attributes(spark::BinaryInStream& stream, const TxID& id, MessageType type);

	// extract a single attribute from a stream
	std::optional<attributes::Attribute>
	extract_attribute(spark::BinaryInStream& stream, const TxID& id, MessageType type);
};

#include <stun/Parser.inl>

} // stun, ember