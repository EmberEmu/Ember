/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Generator.h>
#include <botan/bigint.h>
#include <botan/auto_rng.h>
#include <boost/serialization/strong_typedef.hpp>
#include <boost/container/small_vector.hpp>
#include <span>
#include <string_view>
#include <vector>
#include <cstddef>
 
namespace ember::srp6 {
	
constexpr auto SMALL_VEC_LENGTH = 32;
constexpr auto INTERLEAVE_LENGTH = 40;

using SmallVec = boost::container::small_vector<std::uint8_t, SMALL_VEC_LENGTH>;
using KeyType = boost::container::small_vector<std::uint8_t, INTERLEAVE_LENGTH>;

BOOST_STRONG_TYPEDEF(KeyType, SessionKey);

enum class Compliance { RFC5054, GAME };

namespace detail {

KeyType interleaved_hash(SmallVec key);
SmallVec encode_flip(const Botan::BigInt& val);
SmallVec encode_flip_1363(const Botan::BigInt& val, std::size_t padding);
Botan::BigInt decode_flip(std::span<std::uint8_t> val);
Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding,
                        Compliance mode);
Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N);
Botan::BigInt compute_x(std::string_view identifier, std::string_view password,
                        std::span<const std::uint8_t> salt, Compliance mode);

inline Botan::BigInt compute_v(const Generator& generator, const Botan::BigInt& x) {
	return generator(x);
}

inline Botan::BigInt generate(std::string_view identifier, std::string_view password,
                              const Generator& gen, std::span<const std::uint8_t> salt,
                              Compliance mode) {
	Botan::BigInt x = compute_x(identifier, password, salt, mode);
	return compute_v(gen, x);
}

} // detail

void generate_salt(std::span<std::uint8_t> buffer);

Botan::BigInt generate_verifier(std::string_view identifier, std::string_view password,
                                const Generator& generator, std::span<const std::uint8_t> salt,
                                Compliance mode);

Botan::BigInt generate_client_proof(std::string_view identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g, const Botan::BigInt& A,
                                    const Botan::BigInt& B, std::span<const std::uint8_t> salt);

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key, const std::size_t padding);

} // srp6, ember