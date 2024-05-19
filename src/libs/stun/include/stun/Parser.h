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
#include <stun/Exception.h>
#include <spark/buffers/BinaryStreamReader.h>
#include <boost/endian.hpp>
#include <optional>
#include <array>
#include <span>
#include <vector>
#include <cstddef>

namespace Botan {
	class MessageAuthenticationCode;
} // Botan

namespace ember::stun {

namespace be = boost::endian;

class Parser final {
	RFCMode mode_;
	LogCB logger_ = [](Verbosity, Error) {};
	const std::span<const std::uint8_t> buffer_;

	// individual attributes
	template<typename T> auto extract_ip_pair(spark::BinaryStreamReader& stream);
	template<typename T> auto extract_ipv4_pair(spark::BinaryStreamReader& stream);
	template<typename T> auto extract_utf8_text(spark::BinaryStreamReader& stream, std::size_t size);
	attributes::XorMappedAddress xor_mapped_address(spark::BinaryStreamReader& stream, const TxID& id);
	attributes::UnknownAttributes unknown_attributes(spark::BinaryStreamReader& stream, std::size_t length);
	attributes::ErrorCode error_code(spark::BinaryStreamReader& stream, std::size_t length);
	attributes::MessageIntegrity message_integrity(spark::BinaryStreamReader& stream);
	attributes::MessageIntegrity256 message_integrity_sha256(spark::BinaryStreamReader& stream, std::size_t length);
	attributes::Username username(spark::BinaryStreamReader& stream, std::size_t size);
	attributes::Fingerprint fingerprint(spark::BinaryStreamReader& stream);
	attributes::Priority priority(spark::BinaryStreamReader& stream);
	attributes::IceControlling ice_controlling(spark::BinaryStreamReader& stream);
	attributes::IceControlled ice_controlled(spark::BinaryStreamReader& stream);

	bool check_attr_validity(Attributes attr_type, MessageType msg_type, bool required);

	std::optional<attributes::Attribute> extract_attribute(spark::BinaryStreamReader& stream,
	                                                       const TxID& id, MessageType type);

public:
	Parser(std::span<const std::uint8_t> buffer, RFCMode mode) : buffer_(buffer), mode_(mode) {}
	void set_logger(LogCB logger);

	Error validate_header(const Header& header) const;
	Header header();
	std::vector<attributes::Attribute> attributes();
	std::uint32_t fingerprint() const;
	std::array<std::uint8_t, 20> msg_integrity(std::string_view password) const;

	std::array<std::uint8_t, 20> msg_integrity(std::span<const std::uint8_t> username,
	                                           std::string_view realm,
	                                           std::string_view password) const;
};

#include <stun/Parser.inl>

} // stun, ember