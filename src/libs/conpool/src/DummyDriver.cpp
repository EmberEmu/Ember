/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/DummyDriver.h>
#include <conpool/drivers/DummyConnection.h>
#include <iostream>

namespace ember::drivers {
	
DummyDriver::DummyDriver() {
	std::cout << "DummyDriver opened\n";
}

DummyConnection DummyDriver::open() const {
	std::cout << "DummyConnection opened\n";
	return DummyConnection();
}

DummyConnection DummyDriver::clean(DummyConnection conn) const {
	std::cout << "DummyConnection cleaned\n";
	return conn;
}

void DummyDriver::close(DummyConnection conn) const {
	std::cout << "DummyConnection closed\n";
}


void DummyDriver::clear_state(DummyConnection conn) const {
	std::cout << "DummyConnection state cleared\n";
}

DummyConnection DummyDriver::keep_alive(DummyConnection conn) const {
	std::cout << "DummyConnection keep-alive\n";
	return conn;
}

void DummyDriver::thread_enter() const {
	std::cout << "DummyDriver thread enter\n";
}

void DummyDriver::thread_exit() const {
	std::cout << "DummyDriver thread exit\n";
}

std::string DummyDriver::name() const {
	return "DummyDriver";
}

std::string DummyDriver::version() const {
	return "1.0";
}

} // drivers, ember