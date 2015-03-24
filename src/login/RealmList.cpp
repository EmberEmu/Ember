/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "RealmList.h"
#include <vector>

namespace ember {

RealmList::RealmList(std::vector<Realm> realms) : realms_(std::make_shared<RealmMap>()) {
	add_realm(std::move(realms));
}

void RealmList::add_realm(std::vector<Realm> realms) {
	std::lock_guard<std::mutex> guard(lock_);

	auto copy = std::make_shared<RealmMap>(*realms_);
	
	for(auto& r : realms) {
		copy->emplace(r.id, std::move(r));
	}

	realms_ = copy;
}

void RealmList::add_realm(Realm realm) {
	std::lock_guard<std::mutex> guard(lock_);

	auto copy = std::make_shared<RealmMap>(*realms_);
	copy->emplace(realm.id, std::move(realm));

	realms_ = copy;
}
	
void RealmList::set_status(std::uint32_t id, bool online) {
	std::lock_guard<std::mutex> guard(lock_);

	auto copy = std::make_shared<RealmMap>(*realms_);
}

void RealmList::set_population(float population) {
	std::lock_guard<std::mutex> guard(lock_);

	auto copy = std::make_shared<RealmMap>(*realms_);
}

auto RealmList::realms() const -> std::shared_ptr<const RealmMap> {
	return realms_;
}

} // ember