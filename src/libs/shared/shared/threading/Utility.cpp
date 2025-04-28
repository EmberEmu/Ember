/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include <logger/Logger.h>
#include <shared/CompilerWarn.h>
#include <array>
#include <stdexcept>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <cwchar>
#elif defined TARGET_OS_MAC
#include <pthread.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <signal.h>
#include <atomic>
#elif defined __linux__ || defined __unix__
#include <sched.h>
#include <pthread.h>
#endif

constexpr auto BUFFER_LEN   = 32u;
constexpr auto MAX_NAME_LEN = 16u; // includes null term

namespace ember::thread {

#ifdef _WIN32
typedef HRESULT (__stdcall *SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);
typedef HRESULT (__stdcall *GetThreadDescription)(HANDLE hThread, PWSTR* ppszThreadDescription);
#endif

void set_affinity(auto thread, unsigned int core) {
#ifdef _WIN32
	if(SetThreadAffinityMask(thread, 1u << core) == 0) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(GetLastError()));
	}
#elif defined TARGET_OS_MAC
	// todo, no support for pthread_setaffinity_np - implement when I'm in a position to test it
#elif defined __linux__ || defined __unix__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
	auto ret = pthread_setaffinity_np(thread, sizeof(mask), &mask);

	if(ret) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(ret));
	}
#else
	#pragma message WARN("Setting thread affinity is not implemented for this platform. Implement it, please!")
#endif
}

void set_affinity(std::thread& thread, unsigned int core) {
	set_affinity(thread.native_handle(), core);
}

void set_affinity(std::jthread& thread, unsigned int core) {
	set_affinity(thread.native_handle(), core);
}

Result set_name([[maybe_unused]] auto& handle, const char* name) {
	if(strlen(name) >= MAX_NAME_LEN) {
		throw std::runtime_error("set_name: thread name too long");
	}

#if defined _WIN32 // todo, drop this eventually when bumping minimum Win version
	auto lib = LoadLibrary("Kernel32.dll");

	if(!lib) {
		return Result::unsupported;
	}

	auto set_thread_desc = reinterpret_cast<SetThreadDescription>(GetProcAddress(lib, "SetThreadDescription"));

	if(!set_thread_desc) {
		return Result::unsupported;
	}

	const std::wstring wstr(name, name + strlen(name));
	auto ret = set_thread_desc(handle, wstr.c_str());

	if(FAILED(ret)) {
		FreeLibrary(lib);
		throw std::runtime_error("Unable to set thread name, error code" + std::to_string(ret));
	}

	FreeLibrary(lib);

#elif defined TARGET_OS_MAC
	static std::atomic<const char*> desiredThreadName{name};
	static std::atomic<kern_return_t> ret{KERN_FAILURE};
	static std::atomic<bool> busy{true};

	// Define the signal handler
	const auto threadNameHandler = +[](int /*signum*/) -> void {
		const char* name = desiredThreadName.load(std::memory_order_acquire);
		if (name != nullptr) {
			ret.store(pthread_setname_np(name), std::memory_order_release);
		}
		busy.store(false, std::memory_order_release);
	};

	// Install the signal handler once for the process.
	static std::atomic_flag handlerInstalled = ATOMIC_FLAG_INIT;
	if (!handlerInstalled.test_and_set(std::memory_order_acquire)) {
		signal(SIGUSR1, threadNameHandler);
	}

	// Send SIGUSR1 to the target thread so it runs the signal handler.
	if (pthread_kill(handle, SIGUSR1) != 0) {
		throw std::runtime_error("Unable to send signal handler to thread, error code " + std::to_string(ret));
	}

	while (busy == true) {std::this_thread::yield();}

    if (ret.load(std::memory_order_acquire) != KERN_SUCCESS) {
        throw std::runtime_error("Unable to set thread name, error code " + std::to_string(ret));
    }

#elif defined __linux__ || defined __unix__
	auto ret = pthread_setname_np(handle, name);

	if(ret) {
		throw std::runtime_error("Unable to set thread name, error code" + std::to_string(ret));
	}

#endif

	return Result::ok;
}

Result set_name(std::jthread& thread, const char* name) {
	const auto handle = thread.native_handle();
	return set_name(handle, name);
}

Result set_name(std::thread& thread, const char* name) {
	const auto handle = thread.native_handle();
	return set_name(handle, name);
}

Result set_name(const char* name) {
#ifdef _WIN32
	auto handle = GetCurrentThread();
#else
	auto handle = pthread_self();
#endif
	return set_name(handle, name);
}

std::expected<std::wstring, Result> get_name(auto& thread) {
#if defined _WIN32
	auto lib = LoadLibrary("Kernel32.dll");

	if(!lib) {
		return std::unexpected(Result::unsupported);
	}

	auto get_thread_desc = reinterpret_cast<GetThreadDescription>(GetProcAddress(lib, "GetThreadDescription"));

	if(!get_thread_desc) {
		return std::unexpected(Result::unsupported);
	}

	std::array<wchar_t, BUFFER_LEN> buffer{};
	wchar_t* pbuffer = buffer.data();
	auto res = get_thread_desc(thread, &pbuffer);

	if(FAILED(res)) {
		throw std::runtime_error("Unable to get thread name, error code" + std::to_string(res));
	}

	return std::wstring(buffer.data(), buffer.data() + wcslen(buffer.data()));
#elif defined __linux__ || defined __unix__ || defined TARGET_OS_MAC
	std::array<char, BUFFER_LEN> buffer{};
	auto res = pthread_getname_np(thread, buffer.data(), buffer.size());
	
	if(res) {
		throw std::runtime_error("Unable to get thread name, error code" + std::to_string(res));
	}

	return std::wstring(buffer.data(), buffer.data() + strlen(buffer.data()));
#else
	return std::unexpected(Result::unsupported);
#endif
}

std::expected<std::wstring, Result> get_name(std::jthread& thread) {
	const auto handle = thread.native_handle();
	return get_name(handle);
}

std::expected<std::wstring, Result> get_name(std::thread& thread) {
	const auto handle = thread.native_handle();
	return get_name(handle);
}

std::expected<std::wstring, Result> get_name() {
#ifdef _WIN32
	auto handle = GetCurrentThread();
#else
	auto handle = pthread_self();
#endif
	return get_name(handle);
}

/*
 * The concurrency level returned is usually the number of logical cores
 * in the machine but the standard doesn't guarantee that it won't be zero.
 * In that case, we just set the minimum concurrency level to one.
 */
unsigned int hardware_concurrency(log::Logger* logger) {
	unsigned int concurrency = std::thread::hardware_concurrency();

	if(!concurrency) {
		concurrency = 1;

		if(logger) {
			LOG_WARN_SYNC(logger, "Unable to determine concurrency level, defaulting to {}", concurrency);
		}
	}

	return concurrency;
}

unsigned int hardware_concurrency(log::Logger& logger) {
	return hardware_concurrency(&logger);
}

unsigned int hardware_concurrency() {
	return hardware_concurrency(nullptr);
}

} // thread, ember