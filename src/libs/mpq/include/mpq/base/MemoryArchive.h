/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/Archive.h>
#include <span>
#include <cstddef>

namespace ember::mpq {

class MemoryArchive : public Archive {
	std::span<const std::byte> buffer_;

public:
	MemoryArchive(std::span<const std::byte> buffer) : buffer_(buffer) {}

	int version() const override;
	std::size_t size() const override;
	virtual Backing backing() const override { return Backing::MEMORY; }
};


} // mpq, ember