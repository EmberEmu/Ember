/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Utility.h>
#include <botan/hash.h>
#include <botan/numthry.h>
#include <boost/assert.hpp>
#include <algorithm>
#include <array>
#include <ranges>

constexpr auto sha1_len = 20u;

namespace ember::srp6 {
	
namespace detail {

Botan::BigInt decode_flip(std::span<std::uint8_t> val) {
	std::ranges::reverse(val);
	return Botan::BigInt::from_bytes(val);
}

SmallVector encode_flip(const Botan::BigInt& val) {
	SmallVector res(val.bytes(), boost::container::default_init);
	val.serialize_to(res);
	std::ranges::reverse(res);
	return res;
}

SmallVector encode_flip_1363(const Botan::BigInt& val, std::size_t padding) {
	SmallVector res(padding, boost::container::default_init);
	val.serialize_to(res);
	std::ranges::reverse(res);
	return res;
}

KeyType interleaved_hash(SmallVector key) {
	//implemented as described in RFC2945
	auto begin = std::ranges::find_if(key, [](std::uint8_t b) { return b; });
	begin = std::distance(begin, key.end()) % 2 == 0? begin : begin + 1;

	auto bound = std::stable_partition(begin, key.end(),
	    [&begin](const auto& x) { return (&x - begin.get_ptr()) % 2 == 0; });

	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	BOOST_ASSERT_MSG(sha1_len == hasher->output_length(), "Bad hash length");

	std::array<std::uint8_t, sha1_len> g, h;
	hasher->update(begin.get_ptr(), std::distance(begin, bound));
	hasher->final(g);
	hasher->update(bound.get_ptr(), std::distance(bound, key.end()));
	hasher->final(h);

	KeyType final(interleave_length, boost::container::default_init);

	for(std::size_t i = 0, k = 0, j = g.size(); i < j; ++i) {
		final[k++] = g[i];
		final[k++] = h[i];
	}

	return final;
}

Botan::BigInt scrambler(const Botan::BigInt& A, const Botan::BigInt& B, std::size_t padding,
                        Compliance mode) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");

	std::array<std::uint8_t, sha1_len> hash_out;
	BOOST_ASSERT_MSG(sha1_len == hasher->output_length(), "Bad hash length");

	SmallVector vec(padding, boost::container::default_init);

	if(mode == Compliance::rfc5054) {
		A.serialize_to(vec);
		hasher->update(vec);
		B.serialize_to(vec);
		hasher->update(vec);
		hasher->final(hash_out);
		return Botan::BigInt::from_bytes(hash_out);
	} else {
		const auto& a_enc = encode_flip_1363(A, padding);
		const auto& b_enc = encode_flip_1363(B, padding);
		hasher->update(a_enc);
		hasher->update(b_enc);
		hasher->final(hash_out);
		return decode_flip(hash_out);
	}
}

Botan::BigInt compute_k(const Botan::BigInt& g, const Botan::BigInt& N) {
	//k = H(N, PAD(g)) in SRP6a
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");

	std::array<std::uint8_t, sha1_len> hash;
	BOOST_ASSERT_MSG(sha1_len == hasher->output_length(), "Bad hash length");

	hasher->update(N.serialize());
	hasher->update(g.serialize(N.bytes()));
	hasher->final(hash);
	return Botan::BigInt::from_bytes(hash);
}

Botan::BigInt compute_x(const std::string_view identifier, std::string_view password,
                        std::span<const std::uint8_t> salt, Compliance mode) {
	//RFC2945 defines x = H(s | H ( I | ":" | p) )
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");

	std::array<std::uint8_t, sha1_len> hash;
	BOOST_ASSERT_MSG(hash.size() == hasher->output_length(), "Bad hash length");

	hasher->update(identifier);
	hasher->update(':');
	hasher->update(password);
	hasher->final(hash);

	if(mode == Compliance::rfc5054) {
		hasher->update(salt);
	} else {
		// change if Botan adds iterator overloads
		for(auto i = salt.rbegin(); i != salt.rend(); ++i) {
			hasher->update(*i);
		}
	}

	hasher->update(hash);
	hasher->final(hash);

	if(mode == Compliance::rfc5054) {
		return Botan::BigInt::from_bytes(hash);
	} else {
		return detail::decode_flip(hash);
	}
}

} // detail

Botan::BigInt generate_client_proof(const std::string_view identifier, const SessionKey& key,
                                    const Botan::BigInt& N, const Botan::BigInt& g,
                                    const Botan::BigInt& A, const Botan::BigInt& B,
                                    std::span<const std::uint8_t> salt) {
	//M = H(H(N) xor H(g), H(I), s, A, B, K)
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");

	std::array<std::uint8_t, sha1_len> n_hash, g_hash, i_hash, out;
	BOOST_ASSERT_MSG(sha1_len == hasher->output_length(), "Bad hash length");

	const auto& n_enc = detail::encode_flip(N);
	hasher->update(n_enc);
	hasher->final(n_hash);
	const auto& g_enc = detail::encode_flip(g);
	hasher->update(g_enc);
	hasher->final(g_hash);
	hasher->update(identifier);
	hasher->final(i_hash);
	
	for(auto [n_byte, g_byte] : std::views::zip(n_hash, g_hash)) {
		n_byte ^= g_byte;
	}

	hasher->update(n_hash);
	hasher->update(i_hash);
	const auto& a_enc = detail::encode_flip_1363(A, N.bytes());
	const auto& b_enc = detail::encode_flip_1363(B, N.bytes());

	for(auto byte : salt | std::views::reverse) {
		hasher->update(byte);
	}

	hasher->update(a_enc);
	hasher->update(b_enc);
	hasher->update(key.t);
	hasher->final(out);
	return detail::decode_flip(out);
}

Botan::BigInt generate_server_proof(const Botan::BigInt& A, const Botan::BigInt& proof,
                                    const SessionKey& key, const std::size_t padding) {
	//M = H(A, M, K)
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");

	std::array<std::uint8_t, sha1_len> hash_out;
	BOOST_ASSERT_MSG(sha1_len == hasher->output_length(), "Bad hash length");

	const auto& a_enc = detail::encode_flip_1363(A, padding);
	const auto& proof_enc = detail::encode_flip_1363(proof, sha1_len);
	hasher->update(a_enc);
	hasher->update(proof_enc);
	hasher->update(key.t);
	hasher->final(hash_out);
	return detail::decode_flip(hash_out);
}

void generate_salt(std::span<std::uint8_t> buffer) {
	Botan::AutoSeeded_RNG().randomize(buffer);
}

Botan::BigInt generate_verifier(const std::string_view identifier, std::string_view password,
                                const Generator& generator, std::span<const std::uint8_t> salt,
                                Compliance mode) {
	return detail::generate(identifier, password, generator, salt, mode);
}

} // srp6, ember