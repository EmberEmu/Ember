/*
 * Copyright (c) 2019 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/MulticharConstant.h>
#include <boost/endian/arithmetic.hpp>
#include <format>
#include <string_view>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace be = boost::endian;

namespace ember::dbc {

constexpr auto dbc_magic = utility::make_mcc("WDBC");
constexpr auto dbc_header_size = 20u;

struct Header {
	be::big_uint32_t magic;
	be::little_uint32_t records;
	be::little_uint32_t fields;
	be::little_uint32_t record_size;
	be::little_uint32_t string_block_size;
};

inline void validate_dbc(const std::string_view name, const Header& header, const std::size_t expect_size,
                         const std::size_t expect_fields, const std::size_t dbc_size) {
	if(header.magic != dbc_magic) {
		auto msg = std::format(
			"{}: Invalid header magic - found 0x{:X}, expected 0x{:X}",
			name, header.magic.value(), dbc_magic
		);

		throw std::runtime_error(std::move(msg));
	}

	if(header.record_size != expect_size || header.fields != expect_fields) {
		auto msg = std::format(
			"{}: Expected {} fields, {} byte records but DBC has {} fields and {} byte records",
			name, expect_fields, expect_size, header.fields.value(), header.record_size.value()
		);

		throw std::runtime_error(std::move(msg));
	}

	const std::size_t calculated_size =
		sizeof(Header) + header.string_block_size + (header.record_size * header.records);

	if(calculated_size != dbc_size) {
		auto msg = std::format(
			"{}: Invalid size! Expected {} bytes but the file was {} bytes",
			name, calculated_size, dbc_size
		);

		throw std::runtime_error(std::move(msg));
	}
}

} // dbc, ember