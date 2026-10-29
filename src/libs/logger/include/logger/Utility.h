/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Colour.h>
#include <logger/Exception.h>
#include <logger/Severity.h>
#include <logger/posix/Themes.h>
#include <shared/utility/cstring_view.hpp>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <cstddef>
#include <ctime>

namespace ember::log { 

Severity severity_string(const std::string_view severity);
std::string_view severity_string(Severity severity);


std::istream& operator>>(std::istream& in, Severity& severity);
std::ostream& operator<<(std::ostream& stream, const Severity& severity);

namespace posix {

std::istream& operator>>(std::istream& in, Theme& theme);
std::ostream& operator<<(std::ostream& stream, const Theme& theme);

} // posix

namespace detail {

[[nodiscard]] std::tm current_time();
[[nodiscard]] std::string put_time(const std::tm& time, cstring_view format);
[[nodiscard]] std::size_t put_time(const std::tm& time, cstring_view format, std::span<char> buffer);
[[nodiscard]] Colour severity_colour(Severity severity);
[[nodiscard]] std::string generate_prefix_padding(std::size_t padding_width);
[[nodiscard]] std::size_t calculate_prefix_padding(std::string_view prefix, std::string_view padding);

}} // detail, log, ember