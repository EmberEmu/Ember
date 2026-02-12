/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <optional>
#include <vector>
#include <cstdint>

namespace ember {

struct Realm;

} // ember

namespace ember::dal {

class RealmDAO {
public:
	virtual std::vector<Realm> get_realms() const = 0;
	virtual std::optional<Realm> get_realm(std::uint32_t id) const = 0;
	virtual ~RealmDAO() = default;
};

} // dal, ember