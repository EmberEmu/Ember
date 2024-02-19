/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryInStream.h>
#include <boost/endian.hpp>

template<typename T>
auto Parser::extract_ip_pair(spark::BinaryInStream& stream) {
	stream.skip(1); // skip reserved byte
	T attr{};
	stream >> attr.family;
	stream >> attr.port;

	if(attr.family == AddressFamily::IPV4) {
		attr.family = AddressFamily::IPV4;
		be::big_to_native_inplace(attr.port);
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
	} else if(attr.family == AddressFamily::IPV6) {
		if(mode_ == RFCMode::RFC3489) {
			throw Error::RESP_IPV6_NOT_VALID;
		}

		stream.get(attr.ipv6.begin(), attr.ipv6.end());
		
		for(auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}
	} else {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}
	
	return attr;
}

template<typename T>
auto Parser::extract_ipv4_pair(spark::BinaryInStream& stream) {
	stream.skip(1); // skip padding byte

	T attr{};
	stream >> attr.family;
	stream >> attr.port;
	be::big_to_native_inplace(attr.port);
	stream >> attr.ipv4;
	be::big_to_native_inplace(attr.ipv4);

	if(attr.family != AddressFamily::IPV4) {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}

	return attr;
}

template<typename T>
auto Parser::extract_utf8_text(spark::BinaryInStream& stream, const std::size_t size) {
	// UTF8 encoded sequence of less than 128 characters (which can be as long as 763 bytes)
	if(size > 763) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_SOFTWARE_ATTR);
	}

	T attr{};
	attr.value.resize(size);
	stream.get(attr.value.begin(), attr.value.end());

	// must be padded to the nearest four bytes
	if(auto mod = size % 4) {
		stream.skip(4 - mod);
	}

	return attr;
}
