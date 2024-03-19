/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/v0/MemoryArchive.h>

namespace ember::mpq::v0 {

const Header* MemoryArchive::header() const {
	return std::bit_cast<const Header*, const std::byte*>(buffer_.data());
}

} // v0, mpq, ember