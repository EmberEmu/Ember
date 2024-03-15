/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/MemoryArchive.h>
#include <mpq/Structures.h>
#include <boost/endian/conversion.hpp>
#include <bit>

namespace ember::mpq {

int MemoryArchive::version() const { 
	auto header = std::bit_cast<const v0::Header*, const std::byte*>(buffer_.data());
	return boost::endian::little_to_native(header->format_version);
}

std::size_t MemoryArchive::size() const {
	auto header = std::bit_cast<const v0::Header*, const std::byte*>(buffer_.data());
	return boost::endian::little_to_native(header->archive_size);
}

} // mpq, ember