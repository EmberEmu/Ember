/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <login/IntegrityData.h>
#include <shared/util/FileMD5.h>
#include <gtest/gtest.h>
#include <array>
#include <span>
#include <cstdint>

using namespace ember;

TEST(IntegrityData, LoadData_MD5) {
	std::array<const GameVersion, 1> versions {
		GameVersion {
			.major = 1,
			.minor = 12,
			.build = 5875,
		}	
	};
	
	IntegrityData integrity(versions, "test_data/");
	const auto res = integrity.lookup(versions[0], grunt::Platform::x86,
	                                  grunt::System::Win);
	ASSERT_TRUE(res);
	ASSERT_EQ(res->size_bytes(), 320);

	const auto md5 = util::generate_md5(*res);
	const std::array<std::uint8_t, 16> expected_md5 {
		0xd6, 0x4a, 0xd1, 0xb3, 0x86, 0x02, 0x08, 0x76, 
		0x2d, 0xfd, 0xd1, 0x0a, 0x9b, 0x85, 0x75, 0xbd
	};

	ASSERT_EQ(md5, expected_md5);
}