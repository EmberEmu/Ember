/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <allocators/HugePageAllocator.h>
#include <allocators/Exception.h>
#ifdef __linux__
#include <vector>
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>
#elif defined _WIN32
#define NOMINMAX
#include <Windows.h>
#endif
#include <format>
#include <limits>
#include <new>
#include <cstddef>
#include <cstdlib>

namespace ember::allocators::detail {

#ifdef _WIN32
bool grant_permissions(DWORD& last_error) {
	HANDLE token{};

	auto result = OpenProcessToken(
		GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token
	);

	if(!result) {
		last_error = GetLastError();
		return false;
	}

	TOKEN_PRIVILEGES privileges{};
	privileges.PrivilegeCount = 1;

	result = LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &privileges.Privileges[0].Luid);

	if(!result) {
		last_error = GetLastError();
		CloseHandle(token);
		return false;
	}

	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	result = AdjustTokenPrivileges(
		token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr
	);

	const auto last_err = GetLastError();

	CloseHandle(token);

	if(!result || last_err != ERROR_SUCCESS) {
		last_error = last_err;
		return false;
	}

	return true;
}

#endif

#ifdef _WIN32
void* allocate_win32(const std::size_t size) {
	SYSTEM_INFO info{};
	GetNativeSystemInfo(&info);

	// skip huge pages if the allocation fits within a single page
	if(size <= info.dwPageSize) {
		auto ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if(!ptr) {
			throw std::bad_alloc();
		}

		return ptr;
	}

	DWORD last_error = 0;

	if(!grant_permissions(last_error)) {
		throw huge_page_allocation_error(
			std::format(
				"unable to grant permissions required for huge page creation, error {}", last_error
			)
		);
	}

	const auto page_size = GetLargePageMinimum();

	if(page_size == 0) {
		throw huge_page_allocation_error(
			"system does not support huge pages"
		);
	}

	// round up
	if(size > std::numeric_limits<std::size_t>::max() - (page_size - 1)) {
		throw huge_page_allocation_error(
			"huge page allocation request would overflow"
		);
	}

	const auto aligned_size = ((size + page_size - 1) / page_size) * page_size;

	auto ptr = VirtualAlloc(
		nullptr, aligned_size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE
	);

	if(!ptr) {
		throw std::bad_alloc();
	}

	return ptr;
}
#endif

#ifdef __linux__

#define HUGE_PAGE_MINIMUM 1024 * 1024 * 2 // 2MB - avoiding libhugetlbfs dep

void* allocate_linux(const std::size_t size) {
	// skip huge pages if the allocation fits within a single page
	if(size <= getpagesize()) {
		return ::operator new(size);
	}

	constexpr std::size_t align_to = HUGE_PAGE_MINIMUM;
	const auto rem = size % align_to;
	const auto alloc_size = rem == 0? size : size + (align_to - rem);
	auto allocated = std::aligned_alloc(HUGE_PAGE_MINIMUM, alloc_size);

	if(!allocated) {
		throw std::bad_alloc();
	}

	if(madvise(allocated, alloc_size, MADV_HUGEPAGE) != 0) {
		std::free(allocated);
		throw huge_page_allocation_error(
			std::format(
				"madvise with MADV_HUGEPAGE failed, error {}", errno
			)
		);
	}

	return allocated;
}
#endif

void* huge_page_allocate(const std::size_t size) {
#ifdef __linux__
	return allocate_linux(size);
#elifdef _WIN32
	return allocate_win32(size);
#else
	return ::operator new(size);
#endif
}

void huge_page_deallocate(void* ptr) {
#if defined _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	::operator delete(ptr);
#endif
}

} // allocators, ember