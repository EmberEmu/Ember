/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/Utility.h>
#include <shared/utility/polyfill/start_lifetime_as>
#include <array>
#include <memory>
#include <new>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
	#define _DEBUG_TLS_BLOCK_ALLOCATOR
#endif

namespace ember::spark::io {

enum class PagePolicy {
	no_lock, lock
};

namespace {

struct FreeBlock {
	FreeBlock* next;
};

template<std::size_t size>
concept gt_zero = size > 0;

template<typename _ty>
concept gte_freeblock = sizeof(_ty) >= sizeof(FreeBlock);

template<typename _ty, std::size_t _elements, PagePolicy _policy>
requires gt_zero<_elements> && gte_freeblock<_ty>
struct Allocator {
	struct Block {
		_ty obj;

		struct Metadata {
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(Block);

	std::array<char, block_size * _elements> storage_;
	FreeBlock* head_ = nullptr;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	Allocator() {
		if constexpr(_policy == PagePolicy::lock) {
			util::page_lock(storage_.data(), storage_.size());
		}

		link_blocks();
	}

	void link_blocks() {
		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = std::start_lifetime_as<FreeBlock>(storage + (block_size * i));
			block->next = reinterpret_cast<FreeBlock*>(storage + (block_size * (i + 1)));
		}

		auto tail = reinterpret_cast<FreeBlock*>(storage + (block_size * (_elements - 1)));
		tail->next = nullptr;
		head_ = reinterpret_cast<FreeBlock*>(storage);
	}

	inline void add_block(FreeBlock* block) {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	inline FreeBlock* remove_block(FreeBlock* block) {
		assert(block);
		head_ = block->next;
		return block;
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++total_allocs;
#endif
		Block* block = nullptr;

		if(head_) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++storage_active_count;
#endif
			block = std::start_lifetime_as<Block>(remove_block(head_));
			block->meta.using_new = false;
		} else {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++new_active_count;
#endif
			block = new Block;
			block->meta.using_new = true;
		}

		return new (&block->obj) _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		auto block = std::start_lifetime_as<Block>(t);

		if(block->meta.using_new) [[unlikely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--new_active_count;
			++total_deallocs;
#endif
			t->~_ty();
			delete block;
			return;
		} else {
			t->~_ty();

			auto block = std::start_lifetime_as<FreeBlock>(t);
			add_block(block);

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--storage_active_count;
			++total_deallocs;
#endif
		}
	}

	~Allocator() {
		if constexpr(_policy == PagePolicy::lock) {
			util::page_unlock(storage_.data(), storage_.size());
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !new_active_count);
		assert(total_allocs == total_deallocs);
#endif
	}
};

} // unnamed

template<typename _ty, std::size_t _elements, PagePolicy policy = PagePolicy::lock>
struct TLSBlockAllocator final {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		/*
		 * Can't do this in a ctor unless we can be 100% sure that any object using the
		 * allocator is created on the same thread that ends up using it, otherwise nullptr.
		 * That's probably going to be hassle to keep track of, so we'll just do it here.
		 */
		if(!allocator) {
			allocator = std::make_unique<Allocator<_ty, _elements, policy>>();
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
		++active_allocs;
#endif
		return allocator->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_deallocs;
		--active_allocs;
#endif
		allocator->deallocate(t);
	}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	~TLSBlockAllocator() {
		assert(total_allocs == total_deallocs);
		assert(active_allocs == 0);
	}
#endif

#ifndef _DEBUG_TLS_BLOCK_ALLOCATOR
private:
#endif
	static inline thread_local std::unique_ptr<Allocator<_ty, _elements, policy>> allocator;
};

} // io, spark, ember