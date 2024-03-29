﻿/*  Written in 2016 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#pragma once

#include <cstdint>

namespace ember::rng::xorshift {

std::uint64_t next(void);
extern std::uint64_t seed[2];

} // xorshift, rng, ember