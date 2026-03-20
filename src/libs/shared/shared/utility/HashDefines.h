/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

/*
 * This exists because Botan no longer provides concrete hashing
 * APIs, which means we can't use it to determine hash sizes in bytes
 * for container sizing at compile-time.
 * 
 * This just allows for removing the magic numbers.
 */
namespace ember::hash_sizes {

// todo, use size_t literal when bumping to newer compiler
static constexpr auto crc32  { 4u };
static constexpr auto md5    { 16u };
static constexpr auto sha160 { 20u };
static constexpr auto sha256 { 32u };

} // hash_sizes, ember