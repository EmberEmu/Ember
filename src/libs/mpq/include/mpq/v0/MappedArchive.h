/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/v0/MemoryArchive.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <utility>

namespace ember::mpq::v0 {

class MappedArchive final : public v0::MemoryArchive {
	boost::interprocess::file_mapping file_;
	boost::interprocess::mapped_region region_;

public:
	MappedArchive(boost::interprocess::file_mapping file,
	              boost::interprocess::mapped_region region)
		: v0::MemoryArchive({static_cast<const std::byte*>(region.get_address()), region.get_size()}),
	      file_(std::move(file)), region_(std::move(region)) {}

	Backing backing() const override { return Backing::MAPPED; }
};

} // v0, mpq, ember