/*
 * Copyright (c) 2015 - 2018 Ember
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
#include <vector>
#include <cstdint>

namespace ember {

typedef std::unordered_map<std::uint32_t, Realm> RealmMap;

class RealmList final {
	std::shared_ptr<const RealmMap> realms_;
	mutable std::mutex lock_;

public:
	explicit RealmList(std::vector<Realm> realms);
	RealmList() : realms_(std::make_shared<RealmMap>()){}
	void add_realm(std::vector<Realm> realms);
	void add_realm(Realm realm);
	Realm get_realm(std::uint32_t id) const;
	std::shared_ptr<const RealmMap> realms() const;
};

} // ember