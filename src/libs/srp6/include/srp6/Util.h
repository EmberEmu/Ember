/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Generator.h>
#include <botan/bigint.h>
#include <botan/auto_rng.h>
#include <botan/secmem.h>
#include <boost/serialization/strong_typedef.hpp>
#include <cstddef>
 
namespace ember::srp6 {
	
BOOST_STRONG_TYPEDEF(std::vector<std::uint8_t>, SessionKey);

enum class Compliance { RFC5054, GAME };

namespace detail {

std::vector<std::uint8_t> interleaved_hash(std::vector<std::uint8_t> hash);
std::vector<std::uint8_t> encode_flip(const Botan::BigInt& val);
Botan::secure_vector<std::uint8_t> encode_flip_1363(const Botan::BigInt& val, std::size_t padding);
Botan::BigInt decode_flip(std::vector<std::uint8_t> val);
Botan::BigInt decode_flip(Botan::secure_vector<std::uint8_t> val);
Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding,
                        Compliance mode);
Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N);
Botan::BigInt compute_x(const std::string& identifier, const std::string& password,
                        const Botan::BigInt& salt, Compliance mode);

inline Botan::BigInt compute_v(const Generator& generator, const Botan::BigInt& x) {
	return generator(x);
}

inline Botan::BigInt generate(const std::string& identifier, const std::string& password,
                              const Generator& gen, const Botan::BigInt& salt, Compliance mode) {
	Botan::BigInt x = compute_x(identifier, password, salt, mode);
	return compute_v(gen, x);
}

} //detail

Botan::BigInt generate_salt(std::size_t salt_len);

Botan::BigInt generate_verifier(const std::string& identifier, const std::string& password,
                                const Generator& generator, const Botan::BigInt& salt, Compliance mode);

Botan::BigInt generate_client_proof(const std::string& identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g, const Botan::BigInt& A,
                                    const Botan::BigInt& B, const Botan::BigInt& salt);

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key);

} //srp6, ember