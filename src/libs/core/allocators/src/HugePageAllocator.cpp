/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/HugePageAllocator.h>
#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
#include <hugetlbfs.h>
#include <sys/mman.h>
#include <vector>
#elif defined _WIN32
#define NOMINMAX
#include <Windows.h>
#endif
#include <limits>
#include <cstddef>
#include <cstdlib>

namespace ember::allocators {

#ifdef _WIN32
bool grant_permissions() {
	HANDLE token{};
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	if(!result) {
		return false;
	}

	TOKEN_PRIVILEGES privileges{};
	privileges.PrivilegeCount = 1;

	result = LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &privileges.Privileges[0].Luid);

	if(!result) {
		CloseHandle(token);
		return false;
	}

	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	result = AdjustTokenPrivileges(
		token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr
	);

	const auto last_err = GetLastError();

	CloseHandle(token);

	if(!result|| last_err == ERROR_NOT_ALL_ASSIGNED) {
		return false;
	}

	return true;
}

#endif

#ifdef __linux__
std::size_t determine_page_size(const std::size_t size) {
	if(size == 0) {
		return 0;
	}

	const auto count = gethugepagesizes(nullptr, 0);

	if(count <= 0) {
		return 0;
	}

	std::vector<long> sizes(count);

	if(gethugepagesizes(sizes.data(), count) <= 0) {
		return 0;
	}

	std::size_t result = 0;

	for(const auto size : sizes) {
		if(size <= 0) {
			continue;
		}

		const auto candidate = static_cast<std::size_t>(size);

		if(candidate <= size) {
			result = std::max(result, candidate);
		}
	}

	return result;
}
#endif

#ifdef _WIN32
void* allocate_win32(const std::size_t size) {
	if(!grant_permissions()) {
		return nullptr;
	}

	const auto page_size = GetLargePageMinimum();

	if(page_size == 0) {
		return nullptr;
	}

	// round up
	if(size > std::numeric_limits<std::size_t>::max() - (page_size - 1)) {
		return nullptr;
	}

	const auto aligned_size = ((size + page_size - 1) / page_size) * page_size;

	return VirtualAlloc(
		nullptr, aligned_size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE
	);
}
#endif

#ifdef __linux__
void* allocate_linux(const std::size_t size) {
	constexpr std::size_t align_to = HUGE_PAGE_MINIMUM;
	const auto rem = size % align_to;
	const auto alloc_size = rem == 0? size : size + (align_to - rem);
	auto allocated = std::aligned_alloc(HUGE_PAGE_MINIMUM, alloc_size);

	if(!allocated) {
		return nullptr;
	}

	if(madvise(allocated, alloc_size, MADV_HUGEPAGE) != 0) {
		std::free(allocated);
		return nullptr;
	}

	return allocated;
}
#endif

void* HugePageAllocator::allocate(const std::size_t size) {
#ifdef __linux__
	return allocate_linux(size);
#elif defined _WIN32
	return allocate_win32(size);
#else
	return std::malloc(size);
#endif
}

void HugePageAllocator::deallocate(void* ptr, std::size_t) {
#if defined _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	std::free(ptr);
#endif
}

} // allocators, ember