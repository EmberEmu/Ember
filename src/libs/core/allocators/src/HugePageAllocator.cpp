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
#include <fstream>
#include <cstdint>
#elif defined _WIN32
#define NOMINMAX
#include <Windows.h>
#endif
#include <algorithm>
#include <format>
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
void* allocate_win32(const std::size_t size, const SIZE_T alignment) {
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

	static DWORD last_error = 0;
	static auto result = grant_permissions(last_error);

	if(!result) {
		throw huge_page_allocation_error(
			std::format(
				"unable to grant permissions required for huge page creation, error {}", last_error
			)
		);
	}

	const auto page_size = std::max(GetLargePageMinimum(), alignment);

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

// this is an alternative to using libhugetlbfs or parsing /sys/kernel/mm/hugepages/
// - the downside is that we might not be getting the optimal huge page size
// - libhugetlbfs utils would be easy to include standalone but it's LGPL
std::uint64_t hpage_pmd_size() {
	std::ifstream file("/sys/kernel/mm/transparent_hugepage/hpage_pmd_size");

	std::uint64_t size{};

	if(!(file >> size)) {
		throw std::runtime_error("failed to read hpage_pmd_size");
	}

	return size;
}

void* small_aligned_alloc(const std::size_t size, std::size_t alignment) {
	const auto remainder = size % alignment;
	const auto padding = !remainder? 0 : alignment - remainder;

	if(size > std::numeric_limits<std::size_t>::max() - padding) {
		throw std::bad_alloc();
	}

	const auto aligned_size = ((size + alignment - 1) / alignment) * alignment;
	auto allocated = std::aligned_alloc(alignment, aligned_size);

	if(!allocated) {
		throw std::bad_alloc();
	}

	return allocated;
}

void* allocate_linux(const std::size_t size, const std::size_t alignment) {
	// skip huge pages if the allocation fits within a single page
	if(size <= sysconf(_SC_PAGESIZE)) {
		return small_aligned_alloc(size, alignment);
	}

	static const auto align_to = std::max(
		static_cast<std::size_t>(hpage_pmd_size()), alignment
	);

	const auto remainder = size % align_to;
	const auto padding = remainder == 0 ? 0 : align_to - remainder;

	if(size > std::numeric_limits<std::size_t>::max() - padding) {
		throw std::bad_alloc();
	}

	const auto alloc_size = size + padding;
	auto allocated = std::aligned_alloc(align_to, alloc_size);

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

void* huge_page_allocate(const std::size_t size, const std::size_t alignment) {
#ifdef __linux__
	return allocate_linux(size, alignment);
#elif defined(_WIN32)
	return allocate_win32(size, static_cast<SIZE_T>(alignment));
#else
	return ::operator new(size, std::align_val_t(alignment));
#endif
}

void huge_page_deallocate(void* ptr, const std::size_t /*size*/, const std::size_t alignment) {
#ifdef __linux__
	std::free(ptr);
#elif defined(_WIN32)
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	::operator delete(ptr, std::align_val_t(alignment));
#endif
}

} // allocators, ember