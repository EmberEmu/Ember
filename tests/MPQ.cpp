/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <gtest/gtest.h>

using namespace ember;

TEST(MPQ, FileNotFound) {
	const auto result = mpq::locate_archive("test_data/mpqs/no_such_file");
	ASSERT_EQ(result.error(), mpq::ErrorCode::FILE_NOT_FOUND);
}

TEST(MPQ, ArchiveNotFound) {
	const auto result = mpq::locate_archive("test_data/mpqs/not_an_mpq");
	ASSERT_EQ(result.error(), mpq::ErrorCode::NO_ARCHIVE_FOUND);
}

TEST(MPQ, ArchiveFound) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_0.mpq");
	ASSERT_EQ(*result, 0); // offset
}

TEST(MPQ, ArchiveFoundOffset) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_1.mpq");
	ASSERT_EQ(*result, 0x200); // offset
}

TEST(MPQ, ArchiveNotFoundOffset) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_2.mpq");
	ASSERT_TRUE(!result);
	ASSERT_EQ(result.error(), mpq::ErrorCode::NO_ARCHIVE_FOUND);
}

TEST(MPQ, BadFile_PartialMagic) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_3.mpq");
	ASSERT_TRUE(!result);
	ASSERT_EQ(result.error(), mpq::ErrorCode::NO_ARCHIVE_FOUND);
}

TEST(MPQ, BadHeader_Size) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_5.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_5.mpq", *result), mpq::exception);
}

TEST(MPQ, BadHeader_Version) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_6.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_6.mpq", *result), mpq::exception);
}

TEST(MPQ, BadHeader_Truncated) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_7.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_7.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_HeaderTruncated) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_8.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_8.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_BadArchiveSize) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_9.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_9.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_Success) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_10.mpq");
	ASSERT_TRUE(result);
	ASSERT_NO_THROW(mpq::open_archive("test_data/mpqs/v0_10.mpq", *result));
}

TEST(MPQ, Open_BadHashTableOffset) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_11.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_11.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_BadBlockTableOffset) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_12.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_12.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_BadHashTableSize) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_13.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_13.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_BadBlockTableSize) {
	const auto result = mpq::locate_archive("test_data/mpqs/v0_14.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v0_14.mpq", *result), mpq::exception);
}

TEST(MPQ, Open_BadExtendedBlockTableOffset) {
	const auto result = mpq::locate_archive("test_data/mpqs/v1_15.mpq");
	ASSERT_TRUE(result);
	ASSERT_THROW(mpq::open_archive("test_data/mpqs/v1_15.mpq", *result), mpq::exception);
}