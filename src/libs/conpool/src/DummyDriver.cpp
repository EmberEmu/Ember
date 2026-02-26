/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/DummyDriver.h>
#include <conpool/drivers/DummyConnection.h>
#include <shared/utility/polyfill/print>

namespace ember::drivers {
	
DummyDriver::DummyDriver() {
	std::println("DummyDriver opened");
}

DummyConnection DummyDriver::open() const {
	std::println("DummyConnection opened");
	return DummyConnection();
}

DummyConnection DummyDriver::clean(DummyConnection conn) const {
	std::println("DummyConnection cleaned");
	return conn;
}

void DummyDriver::close(DummyConnection conn) const {
	std::println("DummyConnection closed");
}


void DummyDriver::clear_state(DummyConnection conn) const {
	std::println("DummyConnection state cleared");
}

DummyConnection DummyDriver::keep_alive(DummyConnection conn) const {
	std::println("DummyConnection keep-alive");
	return conn;
}

void DummyDriver::thread_enter() const {
	std::println("DummyDriver thread enter");
}

void DummyDriver::thread_exit() const {
	std::println("DummyDriver thread exit");
}

std::string DummyDriver::name() const {
	return "DummyDriver";
}

std::string DummyDriver::version() const {
	return "1.0";
}

} // drivers, ember