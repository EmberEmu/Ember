/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/impl/Client.h>
#include <ember/blaze/impl/Library.h>
#include <ember/blaze/impl/Symbols.h>
#include <stdexcept>

namespace ember::blaze::impl {

Client::Client() {
	resolve_symbols();
}

Client::~Client() {

}

void Client::resolve_symbols() {
	auto result = library::current_module();

	if(!result) {
		throw std::runtime_error("Unable to retrieve process handle");
	}

	auto log_test = library::find_symbol<log_sync>(*result, "log_sync");

	if(!log_test) {
		throw std::runtime_error("Unable to locate symbol");
	}

	auto fn = *log_test;
	fn("This is a test", 3);
}

void Client::log(int level, const char* message) {
	//
}

void Client::slog(int level, const char* message) {
	//
}

} // impl, blaze, ember