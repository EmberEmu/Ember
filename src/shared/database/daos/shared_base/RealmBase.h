/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/Exception.h>
#include <shared/Realm.h>
#include <boost/optional.hpp>
#include <string>
#include <utility>
#include <vector>

namespace ember { 

namespace dal {

class RealmDAO {
public:
	virtual std::vector<Realm> get_realms() const = 0;
	virtual boost::optional<Realm> get_realm(int id) const = 0;
	virtual ~RealmDAO() = default;
};

}} //dal, ember