/*
 * Copyright (c) 2025 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/game/GameVersion.h>
#include <shared/game/Utility.h>
#include <gtest/gtest.h>

using namespace ember;

TEST(GameVersion, MinorLessThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion lower_version {
		.major = 1,
		.minor = 11,
		.build = 5875,
	};
	
	ASSERT_LT(lower_version, version);
}

TEST(GameVersion, MinorGreaterThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion greater_version {
		.major = 1,
		.minor = 13,
		.build = 5875,
	};

	ASSERT_GT(greater_version, version);
}

TEST(GameVersion, BuildGreaterThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion higher_version {
		.major = 1,
		.minor = 12,
		.build = 5876,
	};

	ASSERT_GT(higher_version, version);
}

TEST(GameVersion, BuildLessThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion lower_version {
		.major = 1,
		.minor = 12,
		.build = 5874,
	};

	ASSERT_LT(lower_version, version);
}

TEST(GameVersion, MajorGreaterThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion higher_version {
		.major = 2,
		.minor = 12,
		.build = 5875,
	};

	ASSERT_GT(higher_version, version);
}

TEST(GameVersion, MajorLessThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion lower_version {
		.major = 0,
		.minor = 12,
		.build = 5875,
	};

	ASSERT_LT(lower_version, version);
}

TEST(GameVersion, Equal) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	const GameVersion equal_version {
		.major = 1,
		.minor = 12,
		.build = 5875,
	};

	ASSERT_EQ(equal_version, version);
}

TEST(GameVersion, GreaterThan) {
	const GameVersion version {
		.major = 1,
		.minor = 12,
		.build = 6001,
	};

	const GameVersion higher_version {
		.major = 1,
		.minor = 13,
		.build = 5875,
	};

	ASSERT_GT(higher_version, version);
}

TEST(GameVersion, LessThan) {
	const GameVersion version {
		.major = 1,
		.minor = 13,
		.build = 5875,
	};

	const GameVersion lower_version {
		.major = 1,
		.minor = 12,
		.build = 6001,
	};

	ASSERT_LT(lower_version, version);
}

TEST(GameVersion, ToString) {
	const GameVersion version {
		.major = 1,
		.minor = 13,
		.build = 5875,
	};

	ASSERT_EQ(to_string(version), "1.13.0 (5875)");

	const GameVersion lower_version {
		.major = 1,
		.minor = 12,
		.build = 6001,
	};

	ASSERT_EQ(to_string(lower_version), "1.12.0 (6001)");
}

TEST(GameVersion, FromStringCommas) {
	const GameVersion expected {
		.major = 1,
		.minor = 12,
		.patch = 1,
		.build = 5875
	};

	GameVersion parsed {};
	const std::string string("1, 12, 1, 5875");
	std::stringstream stream(string);
	stream << string;
	stream >> parsed;

	ASSERT_EQ(expected, parsed);
}

TEST(GameVersion, FromStringPeriods) {
	const GameVersion expected {
		.major = 1,
		.minor = 12,
		.patch = 1,
		.build = 5875
	};

	GameVersion parsed {};
	const std::string string("1.12.1.5875");
	std::stringstream stream(string);
	stream << string;
	stream >> parsed;

	ASSERT_EQ(expected, parsed);
}

TEST(GameVersion, FromStringInvalid) {
	GameVersion parsed {};
	const std::string string("1.12.1 5875");
	std::stringstream stream(string);
	stream >> parsed;

	ASSERT_FALSE(stream);
}