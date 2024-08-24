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
#include <mutex>
#include <cstddef>

namespace ember {

enum ThreadPolicy {
	thread_safe,
	thread_unsafe
};

template<ThreadPolicy _policy = thread_safe>
class ASIOAllocator final {
	constexpr static std::size_t SMALL_SIZE  = 64;
	constexpr static std::size_t MEDIUM_SIZE = 128;
	constexpr static std::size_t LARGE_SIZE  = 256;
	constexpr static std::size_t HUGE_SIZE   = 1024;

	inline boost::pool<>* pool_select(const std::size_t size) {
		if(size <= SMALL_SIZE) {
			return &small_;
		} else if(size <= MEDIUM_SIZE) {
			return &medium_;
		} else if(size <= LARGE_SIZE) {
			return &large_;
		} else if(size <= HUGE_SIZE) {
			return &huge_;
		} else {
			return nullptr;
		}
	}

public:
	ASIOAllocator()
		: small_(SMALL_SIZE),
		  medium_(MEDIUM_SIZE),
		  large_(LARGE_SIZE),
		  huge_(HUGE_SIZE) { }

	ASIOAllocator(const ASIOAllocator&) = delete;
	ASIOAllocator& operator=(const ASIOAllocator&) = delete;

	[[nodiscard]]
	void* allocate(const std::size_t size) {
		boost::pool<>* pool = pool_select(size);

		if(pool) [[likely]] {
			if constexpr(_policy == thread_safe) {
				std::lock_guard guard(m_);
				return pool->malloc();
			} else {
				return pool->malloc();
			}
		} else {
			return ::operator new(size);
		}
	}

	void deallocate(void* chunk, const std::size_t size) {
		boost::pool<>* pool = pool_select(size);

		if(pool) [[likely]] {
			if constexpr(_policy == thread_safe) {
				m_.lock();
			}

			pool->free(chunk);

			if constexpr(_policy == thread_safe) {
				m_.unlock();
			}
		} else {
			::operator delete(chunk);
		}
	}

	boost::pool<> small_, medium_, large_, huge_;
	std::mutex m_;
};

// from the ASIO examples
template <typename Handler, typename Allocator>
class alloc_handler {
public:
	alloc_handler(Allocator& a, Handler&& h)
		: allocator_(a), handler_(std::move(h)) { }

	template <typename ...Args>
	void operator()(Args&&... args) {
		handler_(std::forward<Args>(args)...);
	}

	[[nodiscard]]
	friend void* asio_handler_allocate(std::size_t size,
		alloc_handler<Handler, Allocator>* this_handler) {
		return this_handler->allocator_.allocate(size);
	}

	friend void asio_handler_deallocate(void* pointer, std::size_t size,
		alloc_handler<Handler, Allocator>* this_handler) {
		this_handler->allocator_.deallocate(pointer, size);
	}

private:
	Allocator& allocator_;
	Handler handler_;
};

template <typename Handler, typename Allocator>
inline alloc_handler<Handler, Allocator> create_alloc_handler(Allocator& a, Handler&& h) {
	return alloc_handler(a, std::move(h));
}

} // ember