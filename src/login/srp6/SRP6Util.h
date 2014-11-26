/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SRP6Generator.h"
#include <botan/bigint.h>
#include <botan/auto_rng.h>
#include <boost/serialization/strong_typedef.hpp>
#include <cstddef>
 
namespace SRP6 {
	
BOOST_STRONG_TYPEDEF(Botan::SecureVector<Botan::byte>, SessionKey);

struct SVPair {
	Botan::SecureVector<Botan::byte> salt;
	Botan::BigInt verifier;
};

namespace detail {

Botan::SecureVector<Botan::byte> interleaved_hash(Botan::SecureVector<Botan::byte>& hash);
Botan::SecureVector<Botan::byte> encode_flip(const Botan::BigInt& val);
Botan::BigInt decode_flip(Botan::SecureVector<Botan::byte>& val);
Botan::BigInt decode_flip_copy(const Botan::SecureVector<Botan::byte>& val);
Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B);
Botan::BigInt compute_x(const std::string& identifier, const std::string& password,
                        const Botan::SecureVector<Botan::byte>& salt);

inline Botan::BigInt compute_v(const Generator& generator, const Botan::BigInt& x) {
	return generator(x);
}

inline Botan::BigInt generate(const std::string& identifier, const std::string& password,
                              const Generator& gen, const Botan::SecureVector<Botan::byte>& salt) {
	Botan::BigInt x = compute_x(identifier, password, salt);
	return compute_v(gen, x);
}

} //detail

SVPair generate_verifier(const std::string& identifier, const std::string& password,
	                     const Generator& generator, std::size_t salt_len);

Botan::BigInt generate_client_proof(const std::string& identifier, const SessionKey& key,
	                                const Botan::BigInt& N, const Botan::BigInt& g, const Botan::BigInt& A,
	                                const Botan::BigInt& B, const Botan::SecureVector<Botan::byte>& salt);

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key);

} //SRP6