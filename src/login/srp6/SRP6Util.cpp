/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SRP6Util.h"
#include <botan/sha160.h>
#include <botan/numthry.h>
#include <botan/secmem.h>
#include <algorithm>

using Botan::byte;
using Botan::SecureVector;

namespace SRP6 {
	
namespace detail {

Botan::BigInt decode_flip(SecureVector<byte>& val) {
	std::reverse(val.begin(), val.end());
	return Botan::BigInt::decode(val);
}

SecureVector<byte> encode_flip(const Botan::BigInt& val) {
	SecureVector<Botan::byte> res(Botan::BigInt::encode(val));
	std::reverse(res.begin(), res.end());
	return res;
}

SecureVector<byte> interleaved_hash(SecureVector<byte>& hash) {
	//implemented as described in RFC2945
	auto begin = std::find_if(hash.begin(), hash.end(), [](byte b) { return b; });
	begin = std::distance(begin, hash.end()) % 2 == 0? begin : begin + 1;
	std::size_t index = 0;
	auto bound = std::stable_partition(begin, hash.end(),
		[&index](int) { return ++index % 2; });

	Botan::SHA_160 hasher;
	SecureVector<byte> g(hasher.process(begin, std::distance(begin, bound)));
	SecureVector<byte> h(hasher.process(bound, std::distance(bound, hash.end())));
	SecureVector<byte> final; //todo - when Botan 1.11 works on msvc, .reserve!
	
	for(std::size_t i = 0, j = g.size(); i < j; ++i) {
		final.push_back(g[i]);
		final.push_back(h[i]);
	}

	return final;
}

Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding) {
	Botan::SHA_160 hasher;
	hasher.update(Botan::BigInt::encode_1363(A, padding));
	hasher.update(Botan::BigInt::encode_1363(B, padding));
	return Botan::BigInt::decode(hasher.final());
}

Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N) {
	//k = H(N, PAD(g)) in SRP6a
	Botan::SHA_160 hasher;
	hasher.update(Botan::BigInt::encode(N));
	hasher.update(Botan::BigInt::encode_1363(g, N.bytes()));
	return Botan::BigInt::decode(hasher.final());
}

Botan::BigInt compute_x(const std::string& identifier, const std::string& password,
                        const SecureVector<byte>& salt) {
	//RFC2945 defines x = H(s | H ( I | ":" | p) )
	Botan::SHA_160 hasher;
	hasher.update(identifier);
	hasher.update(":");
	hasher.update(password);
	const SecureVector<byte> hash(hasher.final());

	hasher.update(salt);
	hasher.update(hash);
	return Botan::BigInt::decode(hasher.final());
}

} //detail

Botan::BigInt generate_client_proof(const std::string& identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g,
                                    const Botan::BigInt& A, const Botan::BigInt& B,
                                    const SecureVector<byte>& salt) {
	//M = H(H(N) xor H(g), H(I), s, A, B, K)
	Botan::SHA_160 hasher;
	SecureVector<byte> n_hash(hasher.process(detail::encode_flip(N)));
	SecureVector<byte> g_hash(hasher.process(detail::encode_flip(g)));
	SecureVector<byte> i_hash(hasher.process(identifier));
	
	for(std::size_t i = 0, j = n_hash.size(); i < j; ++i) {
		n_hash[i] ^= g_hash[i];
	}

	hasher.update(n_hash);
	hasher.update(i_hash);
	hasher.update(salt);
	hasher.update(detail::encode_flip(A));
	hasher.update(detail::encode_flip(B));
	hasher.update(key);
	return detail::decode_flip(hasher.final());
}

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key) {
	//M = H(A, M, K)
	Botan::SHA_160 hasher;
	hasher.update(detail::encode_flip(A));
	hasher.update(detail::encode_flip(proof));
	hasher.update(key);
	return detail::decode_flip(hasher.final());
}

SecureVector<byte> generate_salt(std::size_t salt_len) {
	return Botan::AutoSeeded_RNG().random_vec(salt_len);
}

Botan::BigInt generate_verifier(const std::string& identifier, const std::string& password,
                                const Generator& generator, const SecureVector<byte>& salt) {
	return detail::generate(identifier, password, generator, salt);
}

} //SRP6