/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/Endian.h>
#include <concepts>
#include <cstdint>

#pragma once

namespace ember::spark::io {

#define STRING_ADAPTOR(adaptor_name)                                        \
template<                                                                   \
    typename string_type,                                                   \
    std::integral prefix_type = std::uint32_t,                              \
    std::derived_from<endian::storage_tag> endian_tag = endian::as_little_t \
>                                                                           \
struct adaptor_name {                                                       \
    using size_type = prefix_type;                                          \
    static constexpr endian_tag byte_order{};                               \
    string_type& str;                                                       \
    string_type* operator->() { return &str; }                              \
};                                                                          \
/* deduction guide required for clang 17 support */                         \
template<typename string_type>                                              \
adaptor_name(string_type&) -> adaptor_name<string_type>;                    \

STRING_ADAPTOR(raw)
STRING_ADAPTOR(prefixed)
STRING_ADAPTOR(prefixed_varint)
STRING_ADAPTOR(null_terminated)
STRING_ADAPTOR(prefixed_null_terminated)

} // io, spark, ember