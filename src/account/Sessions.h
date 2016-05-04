/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/bigint.h>
#include <boost/optional.hpp>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace ember {

class Sessions {
	bool allow_overwrite_;
	std::unordered_map<std::string, Botan::BigInt> sessions_;
	std::mutex lock_;

public:
	explicit Sessions(bool allow_overwrite) : allow_overwrite_(allow_overwrite) { }
	bool register_session(std::string account, Botan::BigInt key);
	boost::optional<Botan::BigInt> lookup_session(std::string account_id);
};

} // ember