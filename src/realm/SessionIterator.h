/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <iterator>
#include <mutex>

namespace ember::realm {

template<typename _iter_type>
class SessionIterator {
	std::unique_lock<std::mutex> lock_;
	_iter_type it_;

public:
	using iterator_category = std::forward_iterator_tag;
	using difference_type   = std::ptrdiff_t;
	using value_type        = typename _iter_type::value_type;
	using pointer           = typename _iter_type::pointer;
	using reference         = typename _iter_type::reference;

	SessionIterator(_iter_type it, std::mutex& m)
		: lock_(m)
		, it_(it) {}

	SessionIterator(_iter_type it)
		: it_(it) {}

	SessionIterator(const SessionIterator&) = delete;
	SessionIterator& operator=(const SessionIterator&) = delete;

	SessionIterator(SessionIterator&&) = default;
	SessionIterator& operator=(SessionIterator&&) = default;

	reference operator*() {
		return *it_;
	}

	reference operator->() {
		return it_->second;
	}

	SessionIterator& operator++() {
		++it_;
		return *this; 
	}

	bool operator!=(const SessionIterator& other) const {
		return it_ != other.it_; 
	}

	bool operator==(const SessionIterator& other) const {
		return it_ == other.it_; 
	}
};

} // realm, ember