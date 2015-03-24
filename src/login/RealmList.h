/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/Realm.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace ember {

typedef std::unordered_map<std::uint32_t, Realm> RealmMap;

class RealmList {
	std::shared_ptr<const RealmMap> realms_;
	std::mutex lock_;

public:
	RealmList(std::vector<Realm> realms);
	RealmList() : realms_(std::make_shared<RealmMap>()){}
	void add_realm(std::vector<Realm> realms);
	void add_realm(Realm realm);
	void RealmList::set_status(std::uint32_t id, bool online);
	void set_population(float population);
	std::shared_ptr<const RealmMap> realms() const;
};

} // ember