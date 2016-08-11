/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/range/adaptor/map.hpp>

namespace ember { namespace dbc {

template<typename T>
class DBCMap {
	boost::container::flat_map<std::size_t, T> storage;

public:
	template<typename... Args>
	inline void emplace_back(std::size_t id, Args&&... args) {
		storage.emplace(id, std::forward<Args>(args)...);
	}

	inline const T* operator[](std::size_t index) const {
		auto it = storage.find(index);
		
		if(it == storage.end()) {
			return nullptr;
		}

		return &it->second;
	}

	inline auto begin() {
		return storage.begin();
	}

	inline auto end() {
		return storage.end();
	}

	auto values() const {
		return storage | boost::adaptors::map_values;
	}

	auto values()  {
		return storage | boost::adaptors::map_values;
	}
};

}} // dbc, ember