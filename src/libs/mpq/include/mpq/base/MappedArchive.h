/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/MemoryArchive.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace ember::mpq {

class MappedArchive final : public MemoryArchive {
	boost::interprocess::file_mapping file_;
	boost::interprocess::mapped_region region_;

public:
	MappedArchive(boost::interprocess::file_mapping file,
	              boost::interprocess::mapped_region region)
		: MemoryArchive({ static_cast<std::byte*>(region.get_address()), region.get_size() }),
		  file_(std::move(file)), region_(std::move(region)) {}
};


} // mpq, ember