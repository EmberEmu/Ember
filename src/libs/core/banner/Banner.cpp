/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <banner/Banner.h>
#include <banner/Version.h>
#include <logger/ConsoleColour.h>
#include <shared/utility/polyfill/print>

namespace ember {

void print_banner(const std::string_view display_name) {
	log::ConsoleColour console;

	console.set(log::Colour::dark_grey);
	std::println();
	std::println(R"(                                      d8b)");
	console.set(log::Colour::grey);
	std::println(R"(                                      ?88)");
	console.set(log::Colour::yellow);
	std::println(R"(                                       88b)");
	std::println(R"(       )         d8888b  88bd8b,d88b   888888b  d8888b  88bd88b)");
	console.set(log::Colour::light_red);
	std::println(R"(      ) \       d8b_,dP  88P'`?8P'?8b  88P `?8bd8b_,dP  88P'  `)");
	console.set(log::Colour::red);
	std::println(R"(     / ) (      88b     d88  d88  88P d88,  d8888b     d88)");
	std::println(R"(     \(_)/      `?888P'd88' d88'  88bd88'`?88P'`?888P'd88')");
	std::println();

	console.reset();
	std::print("{}, v{} ({}) @ {}, {}\n\n", display_name, build::version, build::git_hash, build::date, build::time);
}

} // ember