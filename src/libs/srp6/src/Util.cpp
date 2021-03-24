/*
 * Copyright (c) 2014 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Util.h>
#include <botan/hash.h>
#include <botan/numthry.h>
#include <boost/assert.hpp>
#include <algorithm>
#include <array>
#include <iostream>

constexpr auto SHA1_LEN = 20;

namespace ember::srp6 {
	
namespace detail {

Botan::BigInt decode_flip(std::span<std::uint8_t> val) {
	std::reverse(val.begin(), val.end());
	return Botan::BigInt::decode(val.data(), val.size());
}

SmallVec encode_flip(const Botan::BigInt& val) {
	SmallVec res;
	res.resize(val.bytes());
	val.binary_encode(res.data(), res.size());
	std::reverse(res.begin(), res.end());
	return res;
}

SmallVec encode_flip_1363(const Botan::BigInt& val, std::size_t padding) {
	SmallVec res(padding);
	Botan::BigInt::encode_1363(res.data(), res.size(), val);
	std::reverse(res.begin(), res.end());
	return res;
}

KeyType interleaved_hash(SmallVec hash) {
	//implemented as described in RFC2945
	auto begin = std::find_if(hash.begin(), hash.end(), [](std::uint8_t b) { return b; });
	begin = std::distance(begin, hash.end()) % 2 == 0? begin : begin + 1;

	auto bound = std::stable_partition(begin, hash.end(),
	    [&begin](const auto& x) { return (&x - &*begin) % 2 == 0; });

	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	BOOST_ASSERT_MSG(SHA1_LEN == hasher->output_length(), "Bad hash length");
	std::array<std::uint8_t, SHA1_LEN> g, h;
	hasher->update(&*begin, std::distance(begin, bound));
	hasher->final(g.data());
	hasher->update(&*bound, std::distance(bound, hash.end()));
	hasher->final(h.data());

	KeyType final(INTERLEAVE_LENGTH);

	for(std::size_t i = 0, k = 0, j = g.size(); i < j; ++i) {
		final[k++] = g[i];
		final[k++] = h[i];
	}

	return final;
}

Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding,
                        Compliance mode) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	BOOST_ASSERT_MSG(SHA1_LEN == hasher->output_length(), "Bad hash length");
	std::array<std::uint8_t, SHA1_LEN> hash_out;

	if(mode == Compliance::RFC5054) {
		hasher->update(Botan::BigInt::encode_1363(A, padding));
		hasher->update(Botan::BigInt::encode_1363(B, padding));
		hasher->final(hash_out.data());
		return Botan::BigInt::decode(hash_out.data(), hash_out.size());
	} else {
		const auto& a_enc = encode_flip_1363(A, padding);
		const auto& b_enc = encode_flip_1363(B, padding);
		hasher->update(a_enc.data(), a_enc.size());
		hasher->update(b_enc.data(), b_enc.size());
		hasher->final(hash_out.data());
		return decode_flip(hash_out);
	}
}

Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N) {
	//k = H(N, PAD(g)) in SRP6a
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	hasher->update(Botan::BigInt::encode(N));
	hasher->update(Botan::BigInt::encode_1363(g, N.bytes()));
	return Botan::BigInt::decode(hasher->final());
}

Botan::BigInt compute_x(const std::string& identifier, const std::string& password,
                        const std::vector<std::uint8_t>& salt, Compliance mode) {
	//RFC2945 defines x = H(s | H ( I | ":" | p) )
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	std::array<std::uint8_t, SHA1_LEN> hash;
	BOOST_ASSERT_MSG(hash.size() == hasher->output_length(), "Bad hash length");
	hasher->update(identifier);
	hasher->update(":");
	hasher->update(password);
	hasher->final(hash.data());

	if(mode == Compliance::RFC5054) {
		hasher->update(salt);
	} else {
		SmallVec salt_enc(salt.begin(), salt.end());
		std::reverse(salt_enc.begin(), salt_enc.end());
		hasher->update(salt_enc.data(), salt_enc.size());
	}

	hasher->update(hash.data(), hash.size());
	hasher->final(hash.data());

	if(mode == Compliance::RFC5054) {
		return Botan::BigInt::decode(hash.data(), hash.size());
	} else {
		return detail::decode_flip(hash);
	}
}

} //detail

Botan::BigInt generate_client_proof(const std::string& identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g,
                                    const Botan::BigInt& A, const Botan::BigInt& B,
                                    const std::vector<std::uint8_t>& salt) {
	//M = H(H(N) xor H(g), H(I), s, A, B, K)
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	std::array<std::uint8_t, SHA1_LEN> n_hash, g_hash, i_hash, out;
	BOOST_ASSERT_MSG(SHA1_LEN == hasher->output_length(), "Bad hash length");
	const auto& n_enc = detail::encode_flip(N);
	hasher->update(n_enc.data(), n_enc.size());
	hasher->final(n_hash.data());
	const auto& g_enc = detail::encode_flip(g);
	hasher->update(g_enc.data(), g_enc.size());
	hasher->final(g_hash.data());
	hasher->update(identifier);
	hasher->final(i_hash.data());
	
	for(std::size_t i = 0, j = n_hash.size(); i < j; ++i) {
		n_hash[i] ^= g_hash[i];
	}

	hasher->update(n_hash.data(), n_hash.size());
	hasher->update(i_hash.data(), i_hash.size());
	const auto& a_enc = detail::encode_flip(A);
	const auto& b_enc = detail::encode_flip(B);
	SmallVec salt_enc(salt.begin(), salt.end());
	std::reverse(salt_enc.begin(), salt_enc.end());
	hasher->update(salt_enc.data(), salt_enc.size());
	hasher->update(a_enc.data(), a_enc.size());
	hasher->update(b_enc.data(), b_enc.size());
	hasher->update(key.t.data(), key.t.size());
	hasher->final(out.data());
	return detail::decode_flip(out);
}

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key) {
	//M = H(A, M, K)
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	BOOST_ASSERT_MSG(SHA1_LEN == hasher->output_length(), "Bad hash length");
	std::array<std::uint8_t, SHA1_LEN> hash_out;
	const auto& a_enc = detail::encode_flip(A);
	const auto& proof_enc = detail::encode_flip(proof);
	hasher->update(a_enc.data(), a_enc.size());
	hasher->update(proof_enc.data(), proof_enc.size());
	hasher->update(key.t.data(), key.t.size());
	hasher->final(hash_out.data());
	return detail::decode_flip(hash_out);
}

std::vector<std::uint8_t> generate_salt(const std::size_t len) {
	const auto rng = Botan::AutoSeeded_RNG().random_vec(len);
	return { rng.begin(), rng.end() };
}

Botan::BigInt generate_verifier(const std::string& identifier, const std::string& password,
                                const Generator& generator, const std::vector<std::uint8_t>& salt,
                                Compliance mode) {
	return detail::generate(identifier, password, generator, salt, mode);
}

} //srp6, ember