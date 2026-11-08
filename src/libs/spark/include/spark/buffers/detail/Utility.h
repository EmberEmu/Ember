/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <limits>
#include <type_traits>
#include <cstddef>

namespace ember::spark::io::detail {

// provides the smallest type required to represent the given size
template<std::size_t size>
using smallest_type =
	std::conditional_t<
		(size <= std::numeric_limits<std::uint8_t>::max()), std::uint8_t,
		std::conditional_t<
			(size <= std::numeric_limits<std::uint16_t>::max()), std::uint16_t,
			std::conditional_t<
				(size <= std::numeric_limits<std::uint32_t>::max()), std::uint32_t,
				std::uint64_t
			>
		>
	>;

} // detail, io, spark, ember