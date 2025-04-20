/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/Realm.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <boost/unordered/unordered_flat_map.hpp>
#include <cstdint>

namespace ember {

using RealmMap = boost::unordered_flat_map<std::uint32_t, Realm>;

class RealmList final {
	std::shared_ptr<const RealmMap> realms_;
	mutable std::mutex lock_;

public:
	explicit RealmList(std::span<const Realm> realms);
	RealmList();

	void add_realm(std::span<const Realm> realms);
	void add_realm(Realm realm);
	std::optional<Realm> get_realm(std::uint32_t id) const;
	std::shared_ptr<const RealmMap> realms() const;
};

} // ember