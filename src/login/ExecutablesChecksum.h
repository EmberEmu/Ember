/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/secmem.h>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::client_integrity {

Botan::secure_vector<Botan::byte> checksum(const Botan::secure_vector<Botan::byte>& seed,
                                           const std::vector<char>* buffer);

Botan::secure_vector<Botan::byte> finalise(const Botan::secure_vector<Botan::byte>& checksum,
                                           const std::uint8_t* seed, std::size_t len);

} // client_integrity, ember