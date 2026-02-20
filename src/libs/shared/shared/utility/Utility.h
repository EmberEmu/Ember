/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "cstring_view.hpp"
#include <chrono>
#include <locale>
#include <string>
#include <string_view>
#include <cstddef>

// fwd decl
namespace Botan { class BigInt; }

/*
 * A small collection of random, mostly platform-specific,
 * utility functions
 */
namespace ember::utility {

std::size_t max_consecutive(const std::string_view name, bool case_insensitive = false,
                            const std::locale& locale = std::locale());

void set_window_title(cstring_view title);

/*
 * Attempts to determine the maximum number of sockets
 * the process is allowed to open.
 */
int max_sockets();

/*
 * Produces a string describing the maximum number of
 * sockets the process is allowed to open.
 */
std::string max_sockets_desc();

/*
 * Converts a signal number to a string
 * e.g. 2 -> SIGINT
 * Values are platform dependent
 */
std::string sig_str(int signal);

/*
 * Locks pages to physical memory, preventing swapping out
 * to disk. Behaviour is platform dependent and error checking
 * must be done based on the platform. Not unit tested because
 * it behaves differently across multiple versions of Windows,
 * requiring the minimum working set size to be increased.
 */
bool page_lock(void* address, std::size_t length);
bool page_unlock(void* address, std::size_t length);


// Botan's deprecated this, so just copying their implementation here
std::uint32_t to_u32bit(const Botan::BigInt& value);

std::string time_duration_format(std::chrono::nanoseconds uptime);

} // utility, ember
