/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <vector>
#include <type_traits>
#include <cstdint>
#include <unordered_map>

namespace ember { namespace dbc {

template<typename T>
class DBCMap {
	std::vector<T> storage;
	std::unordered_map<std::uint32_t, std::size_t> lookup;

public:
	template<typename... Args>
	inline void emplace_back(Args&&... args) {
		storage.emplace_back(std::forward<Args>(args)...);
		lookup[storage.back().id] = storage.size() - 1;
	}

	inline void push_back(const T& object) {
		storage.push_back(object);
		lookup[storage.back().id] = storage.size() - 1;
	}

	inline const T* operator[](std::size_t index) const  {
		auto it = lookup.find(index);
		
		if(it == lookup.end()) {
			return nullptr;
		}

		return &storage[it->second];
	}

	inline auto begin() -> decltype(storage.begin()) const {
		return storage.begin();
	}

	inline auto end() -> decltype(storage.end()) const {
		return storage.end();
	}
};

}} //dbc, ember