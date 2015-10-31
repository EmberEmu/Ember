/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <login/Patcher.h>
#include <gtest/gtest.h>

using namespace ember;

std::vector<GameVersion> versions {
	{ 1, 2, 1, 5875 },
	{ 1, 2, 1, 6005 }
};

TEST(PatcherTest, VersionChecks) {
	GameVersion version_ok { 1, 2, 1, 5875 };
	GameVersion version_too_new { 2, 0, 1, 6180 };
	GameVersion version_too_old { 1, 1, 0, 4044 };
	Patcher patcher(versions, "temp, unused");
	
	ASSERT_EQ(Patcher::PatchLevel::OK, patcher.check_version(version_ok))
		<< "Build should have been accepted";

	ASSERT_EQ(Patcher::PatchLevel::TOO_NEW, patcher.check_version(version_too_new))
		<< "Build should have been rejected as too new";

	ASSERT_EQ(Patcher::PatchLevel::TOO_OLD, patcher.check_version(version_too_old))
		<< "Build should have been rejected as too old";
}