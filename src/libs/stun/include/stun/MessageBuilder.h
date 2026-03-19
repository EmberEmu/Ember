/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Attributes.h>
#include <stun/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::stun {

class MessageBuilder final {
	static constexpr std::size_t buffer_reserve = 4096u;

	using BufferAdaptor = spark::io::BufferAdaptor<std::vector<std::uint8_t>>;
	using BinaryStream = spark::io::BinaryStream<BufferAdaptor>;

	std::vector<std::uint8_t> buffer_;
	BufferAdaptor adaptor_;
	BinaryStream stream_;
	bool finalised_ = false;
	std::size_t key_ = 0;

	void initiate(MessageType type, RFCMode mode);
	void write_padding(std::size_t length);
	be::big_uint16_t len_be(std::size_t length) const;
	be::big_uint16_t attr_be(Attributes attr) const;
	void generate_tx_id(Header& header) const;
	void write_header(Header& header);
	Header build_header(MessageType type, RFCMode mode);
	void set_header_length(std::uint16_t length);

	void add_fingerprint();
	void add_message_integrity(std::span<const std::uint8_t> hash);

	template<typename T>
	void add_textual(Attributes attr, std::span<const T> value);

public:
	MessageBuilder(MessageType type, RFCMode mode);

	std::size_t key() const;
	void add_software(const std::string_view value);
	void add_change_request(bool ip, bool port);
	void add_response_port(std::uint16_t port);

	std::vector<std::uint8_t> final(bool fingerprint = false);

	std::vector<std::uint8_t> final(const char* const password,
	                                bool fingerprint = false);

	std::vector<std::uint8_t> final(const std::string_view password,
	                                bool fingerprint = false);

	std::vector<std::uint8_t> final(std::span<const std::uint8_t> username,
	                                std::string_view realm,
	                                std::string_view password,
	                                bool fingerprint = false);
};

} // stun, ember