/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <iterator>
#include <cstddef>

namespace ember {

template<typename Container>
class unchecked_back_insert_iterator final {
	Container& container_;

public:
    using iterator_category = std::output_iterator_tag;
    using container_type = Container;
	using difference_type = std::ptrdiff_t;

	constexpr explicit unchecked_back_insert_iterator(Container& container) noexcept
		: container_(container) {}

    constexpr unchecked_back_insert_iterator& operator=(const typename Container::value_type& value) {
		container_.unchecked_push_back(std::move(value));
        return *this;
    }

	constexpr unchecked_back_insert_iterator& operator=(typename Container::value_type&& value) {
		container_.unchecked_push_back(std::move(value));
        return *this;
    }

	constexpr unchecked_back_insert_iterator& operator++() noexcept {
		return *this;
	}

	constexpr unchecked_back_insert_iterator operator++(int) noexcept {
		return *this;
	}

	constexpr unchecked_back_insert_iterator& operator*() noexcept {
        return *this;
    }
};

template<typename Container>
constexpr auto unchecked_back_inserter(Container& container) noexcept {
	return unchecked_back_insert_iterator<Container>(container);
}

} // ember