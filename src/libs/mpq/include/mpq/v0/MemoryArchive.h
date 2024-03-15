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

namespace ember::mpq::v0 {

class MemoryArchive : public mpq::MemoryArchive {
public:
	MemoryArchive(std::span<std::byte> buffer) : mpq::MemoryArchive(buffer) {}
	const Header* header() const;
};

} // mpq, ember