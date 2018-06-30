/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Util.h>
#include <botan/sha160.h>
#include <botan/numthry.h>
#include <botan/secmem.h>
#include <algorithm>

using Botan::secure_vector;

namespace ember::srp6 {
	
namespace detail {

Botan::BigInt decode_flip(std::vector<std::uint8_t> val) {
	std::reverse(val.begin(), val.end());
	return Botan::BigInt::decode(val);
}

Botan::BigInt decode_flip(secure_vector<std::uint8_t> val) {
	std::reverse(val.begin(), val.end());
	return Botan::BigInt::decode(val);
}

std::vector<std::uint8_t> encode_flip(const Botan::BigInt& val) {
	std::vector<std::uint8_t> res(Botan::BigInt::encode(val));
	std::reverse(res.begin(), res.end());
	return res;
}

secure_vector<std::uint8_t> encode_flip_1363(const Botan::BigInt& val, std::size_t padding) {
	secure_vector<std::uint8_t> res(Botan::BigInt::encode_1363(val, padding));
	std::reverse(res.begin(), res.end());
	return res;
}

std::vector<std::uint8_t> interleaved_hash(std::vector<std::uint8_t> hash) {
	//implemented as described in RFC2945
	auto begin = std::find_if(hash.begin(), hash.end(), [](std::uint8_t b) { return b; });
	begin = std::distance(begin, hash.end()) % 2 == 0? begin : begin + 1;

	auto bound = std::stable_partition(begin, hash.end(),
	    [&begin](const auto& x) { return (&x - &*begin) % 2 == 0; });

	Botan::SHA_160 hasher;
	secure_vector<std::uint8_t> g(hasher.process(&*begin, std::distance(begin, bound)));
	secure_vector<std::uint8_t> h(hasher.process(&*bound, std::distance(bound, hash.end())));
	std::vector<std::uint8_t> final;
	final.reserve(g.size());

	for(std::size_t i = 0, j = g.size(); i < j; ++i) {
		final.push_back(g[i]);
		final.push_back(h[i]);
	}

	return final;
}

Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding,
                        Compliance mode) {
	Botan::SHA_160 hasher;

	if(mode == Compliance::RFC5054) {
		hasher.update(Botan::BigInt::encode_1363(A, padding));
		hasher.update(Botan::BigInt::encode_1363(B, padding));
		return Botan::BigInt::decode(hasher.final());
	} else {
		hasher.update(encode_flip_1363(A, padding));
		hasher.update(encode_flip_1363(B, padding));
		return decode_flip(hasher.final());
	}
}

Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N) {
	//k = H(N, PAD(g)) in SRP6a
	Botan::SHA_160 hasher;
	hasher.update(Botan::BigInt::encode(N));
	hasher.update(Botan::BigInt::encode_1363(g, N.bytes()));
	return Botan::BigInt::decode(hasher.final());
}

Botan::BigInt compute_x(const std::string& identifier, const std::string& password,
                        const Botan::BigInt& salt, Compliance mode) {
	//RFC2945 defines x = H(s | H ( I | ":" | p) )
	Botan::SHA_160 hasher;
	hasher.update(identifier);
	hasher.update(":");
	hasher.update(password);
	const secure_vector<std::uint8_t> hash(hasher.final());

	if(mode == Compliance::RFC5054) {
		hasher.update(Botan::BigInt::encode(salt));
	} else {
		hasher.update(detail::encode_flip(salt));
	}

	hasher.update(hash);
	return (mode == Compliance::RFC5054)? Botan::BigInt::decode(hasher.final())
	                                      : detail::decode_flip(hasher.final());
}

} //detail

Botan::BigInt generate_client_proof(const std::string& identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g,
                                    const Botan::BigInt& A, const Botan::BigInt& B,
                                    const Botan::BigInt& salt) {
	//M = H(H(N) xor H(g), H(I), s, A, B, K)
	Botan::SHA_160 hasher;
	secure_vector<std::uint8_t> n_hash(hasher.process(detail::encode_flip(N)));
	secure_vector<std::uint8_t> g_hash(hasher.process(detail::encode_flip(g)));
	secure_vector<std::uint8_t> i_hash(hasher.process(identifier));
	
	for(std::size_t i = 0, j = n_hash.size(); i < j; ++i) {
		n_hash[i] ^= g_hash[i];
	}

	hasher.update(n_hash);
	hasher.update(i_hash);
	hasher.update(detail::encode_flip(salt));
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

Botan::BigInt generate_salt(std::size_t salt_len) {
	return Botan::BigInt::decode(Botan::AutoSeeded_RNG().random_vec(salt_len));
}

Botan::BigInt generate_verifier(const std::string& identifier, const std::string& password,
                                const Generator& generator, const Botan::BigInt& salt,
                                Compliance mode) {
	return detail::generate(identifier, password, generator, salt, mode);
}

} //srp6, ember