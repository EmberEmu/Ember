/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/Exception.h>
#include <shared/database/objects/Character.h>
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace ember { namespace dal {

class CharacterDAO {
public:
	virtual boost::optional<Character> character(std::uint64_t id) const = 0;
	virtual boost::optional<Character> character(const std::string& name, std::uint32_t realm_id) const = 0;
	virtual std::vector<Character> characters(std::uint32_t account_id, std::uint32_t realm_id) const = 0;
	virtual void delete_character(std::uint64_t id) const = 0;
	virtual void create(const Character& character) const = 0;
	virtual void update(const Character& character) const = 0;
	virtual ~CharacterDAO() = default;
};

}} // dal, ember