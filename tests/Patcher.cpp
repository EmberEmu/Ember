/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <login/Patcher.h>
#include <shared/database/daos/shared_base/PatchBase.h>
#include <gtest/gtest.h>
#include <numeric>
#include <span>
#include <vector>
#include <cstdint>

using namespace ember;

class MockPatchDAO : public dal::PatchDAO {
public:
	mutable int update_count = 0;
	mutable bool rollup;

	MockPatchDAO(bool include_rollup) : rollup(include_rollup) {}

	std::vector<PatchMeta> fetch_patches() const override {
		std::vector<PatchMeta> patches {
			{0, 1, 2, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "1_to_2.patch", {}, 0},
			{1, 2, 3, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "2_to_3.patch", {}, 0},
			{2, 3, 4, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "3_to_4.patch", {}, 0}
		};

		if(rollup) {
			patches.emplace_back(
				PatchMeta{ 3, 1, 4, 0, 0, 0, "x86", "enGB", "Win", false, true,  "", "1_to_4.patch", {}, 0 }
			);
		}

		return patches;
	};

	void update(const PatchMeta& meta) const override {
		++update_count;
	}
};

std::vector<GameVersion> versions {
	{ 1, 2, 1, 5875 },
	{ 1, 2, 1, 6005 }
};

TEST(PatcherTest, VersionChecks) {
	GameVersion version_ok { 1, 2, 1, 5875 };
	GameVersion version_too_new { 2, 0, 1, 6180 };
	GameVersion version_too_old { 1, 1, 0, 4044 };
	Patcher patcher(versions, std::vector<PatchMeta>());
	
	ASSERT_EQ(Patcher::PatchLevel::OK, patcher.check_version(version_ok))
		<< "Build should have been accepted";

	ASSERT_EQ(Patcher::PatchLevel::TOO_NEW, patcher.check_version(version_too_new))
		<< "Build should have been rejected as too new";

	ASSERT_EQ(Patcher::PatchLevel::TOO_OLD, patcher.check_version(version_too_old))
		<< "Build should have been rejected as too old";
}

TEST(PatcherTest, LoadMD5) {
	MockPatchDAO dao(true);
	const auto meta = Patcher::load_patches("test_data/patches/", dao, nullptr);
	ASSERT_EQ(dao.update_count, 4);
	ASSERT_EQ(meta.size(), 4);

	// std::byte was a mistake
	const std::array<std::byte, 16> md5_1_to_2 {
		std::byte{0x55}, std::byte{0xa5}, std::byte{0x40}, std::byte{0x08},
		std::byte{0xad}, std::byte{0x1b}, std::byte{0xa5}, std::byte{0x89},
		std::byte{0xaa}, std::byte{0x21}, std::byte{0x0d}, std::byte{0x26},
		std::byte{0x29}, std::byte{0xc1}, std::byte{0xdf}, std::byte{0x41}
	};

	const std::array<std::byte, 16> md5_2_to_3 {
		std::byte{0x9e}, std::byte{0x68}, std::byte{0x8c}, std::byte{0x58},
		std::byte{0xa5}, std::byte{0x48}, std::byte{0x7b}, std::byte{0x8e},
		std::byte{0xaf}, std::byte{0x69}, std::byte{0xc9}, std::byte{0xe1},
		std::byte{0x00}, std::byte{0x5a}, std::byte{0xd0}, std::byte{0xbf}
	};

	const std::array<std::byte, 16> md5_3_to_4 {
		std::byte{0x86}, std::byte{0x66}, std::byte{0x68}, std::byte{0x35},
		std::byte{0x06}, std::byte{0xaa}, std::byte{0xcd}, std::byte{0x90},
		std::byte{0x0b}, std::byte{0xbd}, std::byte{0x5a}, std::byte{0x74},
		std::byte{0xac}, std::byte{0x4e}, std::byte{0xdf}, std::byte{0x68}
	};

	const std::array<std::byte, 16> md5_1_to_4 {
		std::byte{0xec}, std::byte{0x7f}, std::byte{0x7e}, std::byte{0x7b},
		std::byte{0xb4}, std::byte{0x37}, std::byte{0x42}, std::byte{0xce},
		std::byte{0x86}, std::byte{0x81}, std::byte{0x45}, std::byte{0xf7},
		std::byte{0x1d}, std::byte{0x37}, std::byte{0xb5}, std::byte{0x3c}
	};

	std::array<bool, 4> matches{};

	for(const auto& m : meta) {
		if(m.file_meta.name == "1_to_2.patch") {
			ASSERT_EQ(m.file_meta.md5, md5_1_to_2);
			ASSERT_EQ(m.file_meta.size, 1);
			matches[0] = true;
		} else if(m.file_meta.name == "2_to_3.patch") {
			ASSERT_EQ(m.file_meta.md5, md5_2_to_3);
			ASSERT_EQ(m.file_meta.size, 1);
			matches[1] = true;
		} else if(m.file_meta.name == "3_to_4.patch") {
			ASSERT_EQ(m.file_meta.md5, md5_3_to_4);
			ASSERT_EQ(m.file_meta.size, 1);
			matches[2] = true;
		} else if(m.file_meta.name == "1_to_4.patch") {
			ASSERT_EQ(m.file_meta.md5, md5_1_to_4);
			ASSERT_EQ(m.file_meta.size, 1);
			matches[3] = true;
		} else {
			ASSERT_TRUE(false) << "Shouldn't have made it here";
		}
	}

	const auto val = std::accumulate(matches.begin(), matches.end(), 0);
	ASSERT_EQ(meta.size(), val);
}

TEST(PatcherTest, IncrementalPatch) {
	const std::vector<GameVersion> supported {
		{ 0, 0, 0, 4 }
	};

	// excluding the rollup patch to test patch pathing as
	// the algorithm will choose the rollup as the optimal path
	// if it finds one (1 patch vs 3)
	MockPatchDAO dao(false);
	const auto meta = Patcher::load_patches("test_data/patches/", dao, nullptr);
	Patcher patcher(supported, meta);

	auto patch = patcher.find_patch(
		GameVersion{ 0, 0, 0, 1 }, grunt::Locale::enGB,
		grunt::Platform::x86, grunt::System::Win
	);

	ASSERT_TRUE(patch);
	ASSERT_EQ(patch->file_meta.name, "1_to_2.patch");

	patch = patcher.find_patch(
		GameVersion{ 0, 0, 0, 2 }, grunt::Locale::enGB,
		grunt::Platform::x86, grunt::System::Win
	);

	ASSERT_TRUE(patch);
	ASSERT_EQ(patch->file_meta.name, "2_to_3.patch");

	patch = patcher.find_patch(
		GameVersion{ 0, 0, 0, 3 }, grunt::Locale::enGB,
		grunt::Platform::x86, grunt::System::Win
	);

	ASSERT_TRUE(patch);
	ASSERT_EQ(patch->file_meta.name, "3_to_4.patch");
}

TEST(PatcherTest, RollupPatch) {
	std::vector<GameVersion> supported {
		{ 0, 0, 0, 4 }
	};

	MockPatchDAO dao(true);
	const auto meta = Patcher::load_patches("test_data/patches/", dao, nullptr);
	Patcher patcher(supported, meta);

	const auto patch = patcher.find_patch(
		GameVersion{ 0, 0, 0, 4 }, grunt::Locale::enGB,
		grunt::Platform::x86, grunt::System::Win
	);

	ASSERT_TRUE(patch);
	ASSERT_EQ(patch->file_meta.name, "1_to_4.patch");
}