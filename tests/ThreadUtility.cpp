/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/


#include <shared/threading/Utility.h>
#include <gtest/gtest.h>
#include <semaphore>
#include <string>
#include <thread>
#include <cstring>

using namespace ember;

// only building these tests for Linux/Unix distros for now
#if defined __linux__ || defined __unix__

TEST(ThreadUtility, Self_GetSetName) {
	const char* set_name = "Test Thread Name";
	thread::set_name(set_name);
	const std::wstring wname(set_name, set_name + strlen(set_name));
	const auto name = thread::get_name();
	ASSERT_EQ(name, wname);
}

TEST(ThreadUtility, GetSetName) {
	std::binary_semaphore sem(0);
	const char* set_name = "Test Thread Name";
	const std::wstring wname(set_name, set_name + strlen(set_name));

	std::thread thread([&]() {
		sem.acquire();
	});

	thread::set_name(thread, set_name);
	const auto name = thread::get_name(thread);
	ASSERT_EQ(name, wname);
	sem.release();
	thread.join();
}

#endif