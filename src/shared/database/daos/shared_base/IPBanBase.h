/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/Exception.h>
#include <boost/optional.hpp>
#include <string>
#include <utility>
#include <vector>

namespace ember { 

typedef std::pair<std::string, int> IPEntry;

namespace dal {

class IPBanDAO {
public:
	virtual boost::optional<int> get_mask(const std::string& ip) = 0;
	virtual std::vector<IPEntry> all_bans() = 0;
	virtual void ban(const IPEntry& ban) = 0;
};

}} //dal, ember