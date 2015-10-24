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
#include <cstdint>

namespace ember { 

typedef std::pair<std::string, std::uint32_t> IPEntry;

namespace dal {

class IPBanDAO {
public:
	virtual boost::optional<std::uint32_t> get_mask(const std::string& ip) const = 0;
	virtual std::vector<IPEntry> all_bans() const = 0;
	virtual void ban(const IPEntry& ban) const = 0;
	virtual ~IPBanDAO() = default;
};

}} //dal, ember