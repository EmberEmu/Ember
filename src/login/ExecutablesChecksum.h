/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/bigint.h>
#include <botan/secmem.h>
#include <initializer_list>
#include <string>
#include <vector>
#include <cstdint>

namespace ember {

class ExecutableChecksum {
	std::vector<std::vector<char>> buffers_;

public:
	ExecutableChecksum(const std::string& path, std::initializer_list<std::string> files);

	Botan::SecureVector<Botan::byte> checksum(const Botan::SecureVector<Botan::byte>& seed) const;

	static Botan::SecureVector<Botan::byte> finalise(const Botan::SecureVector<Botan::byte>& checksum,
	                                                 const std::uint8_t* seed, std::size_t len);
};

} // ember