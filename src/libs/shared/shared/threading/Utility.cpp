/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include <shared/CompilerWarn.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#elif defined TARGET_OS_MAC
#include <pthread.h>
#elif defined __linux__ || defined __unix__
#include <sched.h>
#include <pthread.h>
#endif

namespace ember::thread {

void set_affinity(std::thread& thread, unsigned int core) {	
#ifdef _WIN32
	if(SetThreadAffinityMask(thread.native_handle(), 1u << core) == 0) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(GetLastError()));
	}
#elif defined TARGET_OS_MAC
	// todo, no support for pthread_setaffinity_np - implement when I'm in a position to test it
#elif defined __linux__ || defined __unix__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
	auto ret = pthread_setaffinity_np(thread.native_handle(), sizeof(mask), &mask);

	if(ret) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(ret));
	}
#else
	#pragma message WARN("Setting thread affinity is not implemented for this platform. Implement it, please!");
#endif
}

template<typename T>
void set_name([[maybe_unused]] T& handle, const char* name) {
#ifdef _WIN32
	const std::wstring wstr(name, name + strlen(name));
	auto ret = SetThreadDescription(handle, wstr.c_str());

	if(FAILED(ret)) {
		throw std::runtime_error("Unable to set thread name, error code" + std::to_string(ret));
	}
#elif defined TARGET_OS_MAC
	auto ret = pthread_setname_np(name);

	if(ret) {
		throw std::runtime_error("Unable to set thread name, error code" + std::to_string(ret));
	}
#elif defined __linux__ || defined __unix__
	auto ret = pthread_setname_np(handle, name);

	if(ret) {
		throw std::runtime_error("Unable to set thread name, error code" + std::to_string(ret));
	}
#endif
}

void set_name(std::thread& thread, const char* name) {
#ifndef TARGET_OS_MAC
	const auto handle = thread.native_handle();
	set_name(handle, name);
#else
#pragma message WARN("Setting thread names is not implemented for this platform. Implement it, please!");
#endif
}

void set_name(const char* name) {
#ifdef _WIN32
	auto handle = GetCurrentThread();
	set_name(handle, name);
#elif defined __linux__ || defined __unix__ || defined TARGET_OS_MAC
	auto handle = pthread_self();
	set_name(handle, name);
#else
#pragma message WARN("Setting thread names is not implemented for this platform. Implement it, please!");
#endif
}

template<typename T>
std::wstring get_name(T& thread) {
#ifdef _WIN32
	wchar_t buffer[128]{};
	wchar_t* pbuffer = &buffer[0];
	auto res = GetThreadDescription(thread, &pbuffer);

	if(FAILED(res)) {
		throw std::runtime_error("Unable to get thread name, error code" + std::to_string(res));
	}

	return std::wstring(buffer, buffer + wcslen(buffer));
#elif defined TARGET_OS_MAC
	char buffer[128]{};
	auto res = pthread_getname_np(buffer); // todo, taking a guess here, can't test

	if(res) {
		throw std::runtime_error("Unable to get thread name, error code" + std::to_string(res));
	}

	return std::wstring(buffer, buffer + strlen(buffer));
#elif defined __linux__ || defined __unix__
	char buffer[128]{};
	auto res = pthread_getname_np(thread, buffer);
	
	if(res) {
		throw std::runtime_error("Unable to get thread name, error code" + std::to_string(res));
	}

	return std::wstring(buffer, buffer + strlen(buffer));
#else
	return {}; // default, not implemented
#endif
}

std::wstring get_name(std::thread& thread) {
#ifndef TARGET_OS_MAC
	const auto handle = thread.native_handle();
	return get_name(handle);
#else
	// not implemented, MacOS POSIX functions don't take a thread arg (might for getter, not sure)
	return {};
#endif
}

std::wstring get_name() {
#ifdef _WIN32
	auto handle = GetCurrentThread();
	return get_name(handle);
#elif defined __linux__ || defined __unix__ || TARGET_OS_MAC
	auto handle = pthread_self();
	return get_name(handle);
#else
#pragma message WARN("Getting thread names is not implemented for this platform. Implement it, please!");
	return get_name(0); // just return a default, empty string
#endif
}

} // thread, ember