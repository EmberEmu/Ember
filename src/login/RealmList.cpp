/*
 * Copyright (c) 2015 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmList.h"
#include <utility>

namespace ember {

RealmList::RealmList(std::vector<Realm>&& realms) : realms_(std::make_shared<RealmMap>()) {
	add_realm(std::move(realms));
}

void RealmList::add_realm(std::vector<Realm> realms) {
	// ensure consistency if we add from multiple workers (not a thread safety issue)
	std::lock_guard<std::mutex> guard(lock_);

	auto realm_map = realms_.load();
	auto copy = std::make_shared<RealmMap>(*realm_map);
	
	for(auto& realm : realms) {
		(*copy)[realm.id] = std::move(realm);
	}

	realms_ = copy;
}

void RealmList::add_realm(Realm realm) {
	// ensure consistency if we add from multiple workers (not a thread safety issue)
	std::lock_guard<std::mutex> guard(lock_);

	auto realm_map = realms_.load();
	auto copy = std::make_shared<RealmMap>(*realm_map);
	(*copy)[realm.id] = std::move(realm);

	realms_ = copy;
}

Realm RealmList::get_realm(const std::uint32_t id) const {
	auto realms = realms_.load();
	return realms->at(id);
}

auto RealmList::realms() const -> std::shared_ptr<const RealmMap> {
	auto realms = realms_.load();
	return realms;
}

} // ember