/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <botan/secmem.h>
#include <string>
#include <cstddef>

namespace ember::util {

Botan::secure_vector<Botan::byte> generate_md5(const std::byte* data, const std::size_t len);
Botan::secure_vector<Botan::byte> generate_md5(const std::string& file);

} // util, ember