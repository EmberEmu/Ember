/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <array>
#include <login/RealmList.h>
#include <gtest/gtest.h>

using namespace ember;

TEST(RealmList, ZeroRealms) {
	RealmList list;
	auto realms = list.realms();
	ASSERT_EQ(realms->empty(), true);
}

TEST(RealmList, AddRealmsCtor) {
	const std::array<Realm, 2> init_realms {{
		{ .id = 0, .name = "Test Realm #1" },
		{ .id = 1, .name = "Test Realm #2" },
	}};

	RealmList list(init_realms);
	auto realms = list.realms();
	ASSERT_EQ(realms->size(), init_realms.size());
	ASSERT_EQ(realms->at(0).name, "Test Realm #1");
	ASSERT_EQ(realms->at(1).name, "Test Realm #2");
}

TEST(RealmList, AddRealms) {
	RealmList list;
	list.add_realm({ .id = 0, .name = "Test Realm #1" });
	list.add_realm({ .id = 1, .name = "Test Realm #2" });
	auto realms = list.realms();
	ASSERT_EQ(realms->size(), 2);
	ASSERT_EQ(realms->at(0).name, "Test Realm #1");
	ASSERT_EQ(realms->at(1).name, "Test Realm #2");
}

TEST(RealmList, AddRealmsSpan) {
	const std::array<Realm, 2> init_realms {{
		{ .id = 0, .name = "Test Realm #1" },
		{ .id = 1, .name = "Test Realm #2" },
	}};

	RealmList list;
	list.add_realm(init_realms);
	auto realms = list.realms();
	ASSERT_EQ(realms->size(), 2);
	ASSERT_EQ(realms->at(0).name, "Test Realm #1");
	ASSERT_EQ(realms->at(1).name, "Test Realm #2");
}

TEST(RealmList, AddRealmUpdate) {
	RealmList list;

	list.add_realm({ .id = 0, .name = "Test Realm #1" });
	auto realms = list.realms();
	ASSERT_EQ(realms->size(), 1);
	ASSERT_EQ(realms->at(0).name, "Test Realm #1");

	list.add_realm({ .id = 0, .name = "Test Realm #2" });
	realms = list.realms();
	ASSERT_EQ(realms->size(), 1);
	ASSERT_EQ(realms->at(0).name, "Test Realm #2");
}

TEST(RealmList, GetRealm) {
	const Realm init_realm {
		.id = 5,
		.name = "Zenedar" 
	};

	RealmList list;
	list.add_realm(init_realm);
	auto realm = list.get_realm(5);
	ASSERT_EQ(realm.has_value(), true);
	ASSERT_EQ(realm->id, init_realm.id);
	ASSERT_EQ(realm->name, init_realm.name);
}

TEST(RealmList, GetInvalidRealm) {
	RealmList list;
	list.add_realm({ .id = 0, .name = "Test Realm #1" });
	auto realm = list.get_realm(1);
	ASSERT_EQ(realm.has_value(), false);
}