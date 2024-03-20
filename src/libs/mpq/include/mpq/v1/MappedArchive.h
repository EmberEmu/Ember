/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/v1/MemoryArchive.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <utility>

namespace ember::mpq::v1 {

class MappedArchive final : public v1::MemoryArchive {
	boost::interprocess::file_mapping file_;
	boost::interprocess::mapped_region region_;

public:
	MappedArchive(boost::interprocess::file_mapping file,
	              boost::interprocess::mapped_region region)
		: v1::MemoryArchive({static_cast<std::byte*>(region.get_address()), region.get_size()}),
	      file_(std::move(file)), region_(std::move(region)) {}

	Backing backing() const override { return Backing::MAPPED; }
};

} // v1, mpq, ember