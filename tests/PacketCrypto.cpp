/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <realm/PacketCrypto.h>
#include <gtest/gtest.h>
#include <array>
#include <cstdint>

#include <iomanip> // temp

using namespace ember;

struct PacketCryptoTest : public testing::Test {
	std::array<std::uint8_t, 64> key {
		0x0f, 0xa9, 0xde, 0xdc, 0x42, 0xb4, 0x67, 0x0a, 0xfd, 0xa8, 0xae, 0xa0, 0x7e, 0xea, 0xa8, 0x66,
		0x92, 0x80, 0x52, 0x80, 0xf4, 0x07, 0x7c, 0x6f, 0xfc, 0x0a, 0x22, 0x83, 0x18, 0x91, 0x11, 0x8f,
		0x88, 0x6f, 0xc4, 0x23, 0x6d, 0x7c, 0xa0, 0x08, 0xfc, 0xb9, 0x84, 0x15, 0xf4, 0xad, 0x04, 0x10,
		0x02, 0x1b, 0x0b, 0x5d, 0x98, 0xc8, 0x15, 0x5d, 0xbe, 0x03, 0x1e, 0x82, 0xc0, 0x2b, 0xed, 0x68
	};

	std::array<std::uint8_t, 64> input {
		0x57, 0x4f, 0xb5, 0x0a, 0x06, 0x61, 0xcf, 0x71, 0x15, 0xe9, 0x15, 0x56, 0xc4, 0xe9, 0xb2, 0x88,
		0xb4, 0x82, 0x20, 0x06, 0x13, 0xbe, 0xce, 0x60, 0x0a, 0xef, 0xbb, 0x90, 0x6b, 0xba, 0x2a, 0xa4,
		0xb5, 0xcf, 0x60, 0x8b, 0x28, 0xcb, 0x03, 0x1d, 0xc3, 0xd3, 0x94, 0x9a, 0xa5, 0x21, 0x68, 0x9c,
		0x44, 0x8f, 0xec, 0x65, 0x8c, 0x64, 0x8f, 0xfe, 0xec, 0x72, 0xb2, 0xd0, 0xdf, 0x7c, 0x79, 0x8e
	};

	std::array<std::uint8_t, 64> expected_enc {
		0x58, 0x3e, 0xa9, 0x7f, 0xc3, 0x98, 0x40, 0xbb, 0xa3, 0xe4, 0x9f, 0x95, 0x4f, 0x52, 0x6c, 0x5a,
		0x80, 0x82, 0xf4, 0x7a, 0x61, 0x1a, 0xcc, 0xdb, 0xd1, 0xb6, 0x4f, 0x62, 0xd5, 0x00, 0x3b, 0x66,
		0xa3, 0x43, 0xe7, 0x8f, 0xd4, 0x8b, 0x2e, 0x43, 0x82, 0xec, 0xfc, 0x8b, 0xdc, 0x68, 0xd4, 0x60,
		0xa6, 0x3a, 0x21, 0x59, 0x6d, 0x19, 0xb3, 0x56, 0xa8, 0x19, 0xc5, 0x17, 0x36, 0x8d, 0x21, 0x07
	};

	std::array<std::uint8_t, 64> expected_dec {
		0x58, 0x51, 0xb8, 0x89, 0xbe, 0xef, 0x09, 0xa8, 0x59, 0x7c, 0x82, 0xe1, 0x10, 0xcf, 0x61, 0xb0,
		0xbe, 0x4e, 0xcc, 0x66, 0xf9, 0xac, 0x6c, 0xfd, 0x56, 0xef, 0xee, 0x56, 0xc3, 0xde, 0x61, 0xf5,
		0x99, 0x75, 0x55, 0x08, 0xf0, 0xdf, 0x98, 0x12, 0x5a, 0xa9, 0x45, 0x13, 0xff, 0xd1, 0x43, 0x24,
		0xaa, 0x50, 0x56, 0x24, 0xbf, 0x10, 0x3e, 0x32, 0x50, 0x85, 0x5e, 0x9c, 0xcf, 0xb6, 0x10, 0x7d
	};
};

TEST_F(PacketCryptoTest, KeySizeTooBig) {
	std::array<std::uint8_t, 257> _key;
	ASSERT_THROW(realm::PacketCrypto pc(_key), std::runtime_error);
}

TEST_F(PacketCryptoTest, KeyEmpty) {
	std::array<std::uint8_t, 0> _key;
	ASSERT_THROW(realm::PacketCrypto pc(_key), std::runtime_error);
}

TEST_F(PacketCryptoTest, KeySizeMismatchAbove) {
	std::array<std::uint8_t, 65> _key;
	ASSERT_THROW(realm::PacketCrypto<64> pc(_key), std::runtime_error);
}

TEST_F(PacketCryptoTest, KeySizeMismatchBelow) {
	std::array<std::uint8_t, 63> _key;
	ASSERT_THROW(realm::PacketCrypto<64> pc(_key), std::runtime_error);
}

TEST_F(PacketCryptoTest, KeySizeMax) {
	std::array<std::uint8_t, 255> _key;
	ASSERT_NO_THROW(realm::PacketCrypto pc(_key));
}

TEST_F(PacketCryptoTest, KeySizeMin) {
	std::array<std::uint8_t, 1> _key;
	ASSERT_NO_THROW(realm::PacketCrypto pc(_key));
}

TEST_F(PacketCryptoTest, KeySizeExpected) {
	std::array<std::uint8_t, 64> _key;
	ASSERT_NO_THROW(realm::PacketCrypto<64> pc(_key));
}

TEST_F(PacketCryptoTest, EncryptPerByte) {
	realm::PacketCrypto pc(key);

	for(auto& byte : input) {
		pc.encrypt(byte);
	}

	ASSERT_EQ(input, expected_enc);
}

TEST_F(PacketCryptoTest, Encrypt) {
	realm::PacketCrypto pc(key);
	pc.encrypt(input.data(), input.size());
	ASSERT_EQ(input, expected_enc);
}

TEST_F(PacketCryptoTest, DecryptPerByte) {
	realm::PacketCrypto pc(key);

	for(auto& byte : input) {
		pc.decrypt(byte);
	}

	ASSERT_EQ(input, expected_dec);
}

TEST_F(PacketCryptoTest, DecryptMulti) {
	realm::PacketCrypto pc(key);
	pc.decrypt(input.data(), input.size());
	ASSERT_EQ(input, expected_dec);
}

TEST_F(PacketCryptoTest, SizedEncryptSingle) {
	realm::PacketCrypto<64> pc(key);

	for(auto& byte : input) {
		pc.encrypt(byte);
	}

	ASSERT_EQ(input, expected_enc);
}

TEST_F(PacketCryptoTest, SizedEncryptMulti) {
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(input.data(), input.size());
	ASSERT_EQ(input, expected_enc);
}

TEST_F(PacketCryptoTest, SizedDecryptSingle) {
	realm::PacketCrypto<64> pc(key);

	for(auto& byte : input) {
		pc.decrypt(byte);
	}

	ASSERT_EQ(input, expected_dec);
}

TEST_F(PacketCryptoTest, SizedDecryptMulti) {
	realm::PacketCrypto<64> pc(key);
	pc.decrypt(input.data(), input.size());
	ASSERT_EQ(input, expected_dec);
}

TEST_F(PacketCryptoTest, EncryptUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.encrypt(value);
	ASSERT_EQ(value, 0xc029be7a5ce5b02);
}

TEST_F(PacketCryptoTest, DecryptUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.decrypt(value);
	ASSERT_EQ(value, 0xa67b4b782634a02);
}

TEST_F(PacketCryptoTest, RoundTripUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.encrypt(value);
	pc.decrypt(value);
	ASSERT_EQ(value, 0xbadf00d);
}

TEST_F(PacketCryptoTest, SizedEncryptUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(value);
	ASSERT_EQ(value, 0xc029be7a5ce5b02);
}

TEST_F(PacketCryptoTest, SizedDecryptUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.decrypt(value);
	ASSERT_EQ(value, 0xa67b4b782634a02);
}

TEST_F(PacketCryptoTest, SizedRoundTripUint64) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(value);
	pc.decrypt(value);
	ASSERT_EQ(value, 0xbadf00d);
}

TEST_F(PacketCryptoTest, EncryptUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.encrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xc029be7a5ce5b02);
}

TEST_F(PacketCryptoTest, DecryptUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.decrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xa67b4b782634a02);
}

TEST_F(PacketCryptoTest, RoundTripUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto pc(key);
	pc.encrypt(&value, sizeof(value));
	pc.decrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xbadf00d);
}

TEST_F(PacketCryptoTest, SizedEncryptUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xc029be7a5ce5b02);
}

TEST_F(PacketCryptoTest, SizedDecryptUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.decrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xa67b4b782634a02);
}

TEST_F(PacketCryptoTest, SizedRoundTripUint64SingleBytes) {
	std::uint64_t value = 0xbadf00d;
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(&value, sizeof(value));
	pc.decrypt(&value, sizeof(value));
	ASSERT_EQ(value, 0xbadf00d);
}


TEST_F(PacketCryptoTest, RoundTripMulti) {
	std::array<std::uint8_t, 64> expected(input);
	realm::PacketCrypto pc(key);

	for(auto& byte : input) {
		pc.encrypt(byte);
	}

	for(auto& byte : input) {
		pc.decrypt(byte);
	}

	ASSERT_EQ(input, expected);
}

TEST_F(PacketCryptoTest, RoundTripSingle) {
	std::array<std::uint8_t, 64> expected(input);
	realm::PacketCrypto pc(key);
	pc.encrypt(input.data(), input.size());
	pc.decrypt(input.data(), input.size());
	ASSERT_EQ(input, expected);
}

TEST_F(PacketCryptoTest, SizedRoundTripMulti) {
	std::array<std::uint8_t, 64> expected(input);
	realm::PacketCrypto<64> pc(key);

	for(auto& byte : input) {
		pc.encrypt(byte);
	}

	for(auto& byte : input) {
		pc.decrypt(byte);
	}

	ASSERT_EQ(input, expected);
}

TEST_F(PacketCryptoTest, SizedRoundTripSingle) {
	std::array<std::uint8_t, 64> expected(input);
	realm::PacketCrypto<64> pc(key);
	pc.encrypt(input.data(), input.size());
	pc.decrypt(input.data(), input.size());
	ASSERT_EQ(input, expected);
}