/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Attributes.h>
#include <stun/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::stun {

class MessageBuilder final {
	std::vector<std::uint8_t> buffer_;
	spark::VectorBufferAdaptor<std::uint8_t> vba_;
	spark::BinaryStream stream_;
	bool finalised_ = false;
	std::size_t key_ = 0;

	void write_padding(std::size_t length);
	be::big_uint16_t len_be(std::size_t length);
	be::big_uint16_t attr_be(Attributes attr);
	void generate_tx_id(Header& header);
	void write_header(Header& header);
	Header build_header(MessageType type, RFCMode mode);
	void set_header_length(std::uint16_t length);

	void add_fingerprint();
	void add_message_integrity(const std::vector<std::uint8_t>& hash);

	template<typename T>
	void add_textual(Attributes attr, std::span<const T> value);

public:
	MessageBuilder(MessageType type, RFCMode mode);

	std::size_t key() const;
	void add_software(std::string_view value);
	void add_change_request(bool ip, bool port);
	void add_response_port(std::uint16_t port);

	std::vector<std::uint8_t> final(bool fingerprint = false);

	std::vector<std::uint8_t> final(const char* const password,
									bool fingerprint = false);

	std::vector<std::uint8_t> final(std::string_view password,
	                                bool fingerprint = false);

	std::vector<std::uint8_t> final(std::span<const std::uint8_t> username,
	                                std::string_view realm,
	                                std::string_view password,
	                                bool fingerprint = false);
};

} // stun, ember