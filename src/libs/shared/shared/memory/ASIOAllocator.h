/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <cstddef>

namespace ember {

class ASIOAllocator final {
	constexpr static std::size_t SMALL_SIZE_  = 64;
	constexpr static std::size_t MEDIUM_SIZE_ = 128;
	constexpr static std::size_t LARGE_SIZE_  = 256;
	constexpr static std::size_t HUGE_SIZE_   = 1024;

	inline boost::pool<>* pool_select(std::size_t size) {
		if(size <= SMALL_SIZE_) {
			return &small_;
		} else if(size <= MEDIUM_SIZE_) {
			return &medium_;
		} else if(size <= LARGE_SIZE_) {
			return &large_;
		} else if(size <= HUGE_SIZE_) {
			return &huge_;
		} else {
			return nullptr;
		}
	}

public:
	ASIOAllocator() : small_(SMALL_SIZE_), medium_(MEDIUM_SIZE_),
		large_(LARGE_SIZE_), huge_(HUGE_SIZE_) { }

	ASIOAllocator(const ASIOAllocator&) = delete;
	ASIOAllocator& operator=(const ASIOAllocator&) = delete;

	void* allocate(std::size_t size) {
		boost::pool<>* pool = pool_select(size);

		if(pool) {
			return pool->malloc();
		} else {
			return ::operator new(size);
		}
	}

	void deallocate(void* chunk, std::size_t size) {
		boost::pool<>* pool = pool_select(size);

		if(pool) {
			pool->free(chunk);
		} else {
			::operator delete(chunk);
		}
	}

	boost::pool<> small_, medium_, large_, huge_;
};

// from the ASIO examples
template <typename Handler>
class alloc_handler {
public:
	alloc_handler(ASIOAllocator& a, Handler h)
		: allocator_(a), handler_(h) { }

	template <typename ...Args>
	void operator()(Args&&... args) {
		handler_(std::forward<Args>(args)...);
	}

	friend void* asio_handler_allocate(std::size_t size,
		alloc_handler<Handler>* this_handler) {
		return this_handler->allocator_.allocate(size);
	}

	friend void asio_handler_deallocate(void* pointer, std::size_t size,
		alloc_handler<Handler>* this_handler) {
		this_handler->allocator_.deallocate(pointer, size);
	}

private:
	ASIOAllocator& allocator_;
	Handler handler_;
};

template <typename Handler>
inline alloc_handler<Handler> create_alloc_handler(ASIOAllocator& a, Handler h) {
	return alloc_handler<Handler>(a, h);
}

} // ember