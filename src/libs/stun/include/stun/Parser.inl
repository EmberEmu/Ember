/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BinaryStreamReader.h>
#include <boost/endian.hpp>

template<typename T>
auto Parser::extract_ip_pair(spark::io::pmr::BinaryStreamReader& stream) {
	stream.skip(1); // skip reserved byte
	T attr{};
	stream >> attr.family;
	stream >> attr.port;

	if(attr.family == AddressFamily::ipv4) {
		attr.family = AddressFamily::ipv4;
		be::big_to_native_inplace(attr.port);
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
	} else if(attr.family == AddressFamily::ipv6) {
		if(mode_ == RFCMode::rfc3489) {
			throw parse_error(Error::resp_ipv6_not_valid,
				"IPV6 is not valid in this mode");
		}

		stream.get(attr.ipv6);
		
		for(auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}
	} else {
		throw parse_error(Error::resp_addr_fam_not_valid,
			"Invalid address family");
	}
	
	return attr;
}

template<typename T>
auto Parser::extract_ipv4_pair(spark::io::pmr::BinaryStreamReader& stream) {
	stream.skip(1); // skip padding byte

	T attr{};
	stream >> attr.family;
	stream >> attr.port;
	be::big_to_native_inplace(attr.port);
	stream >> attr.ipv4;
	be::big_to_native_inplace(attr.ipv4);

	if(attr.family != AddressFamily::ipv4) {
		throw parse_error(Error::resp_addr_fam_not_valid,
			"Invalid address family");
	}

	return attr;
}

template<typename T>
auto Parser::extract_utf8_text(spark::io::pmr::BinaryStreamReader& stream, const std::size_t size) {
	// UTF8 encoded sequence of less than 128 characters (which can be as long as 763 bytes)
	if(size > 763) {
		logger_(Severity::debug, Error::resp_bad_software_attr);
	}

	T attr{};

	attr.value.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
		stream.get(strbuf, len);
		return len;
	});

	// must be padded to the nearest four bytes
	if(auto mod = size % PADDING_ROUND) {
		const auto skip_size = PADDING_ROUND - mod;

		if(stream.size() < skip_size) {
			throw parse_error(Error::buffer_parse_error,
				"Textual attribute was not padded correcly");
		}

		stream.skip(skip_size);
	}

	return attr;
}
