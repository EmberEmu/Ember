/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <sstream>
#include <iomanip>
#include <string>
#include <cstddef>
#include <cctype>

namespace ember { namespace util {

inline std::string format_packet(unsigned char* packet, std::size_t size) {
	std::stringstream hex, ascii, formatted;
	hex << std::hex << std::setfill('0');

	for(std::size_t i = 1; i != size; ++i) {
		hex << std::setw(2) << static_cast<unsigned>(packet[i]) << " ";
		ascii << (std::isprint(packet[i])? packet[i] : static_cast<unsigned char>('.'));

		if(!(i % 15)) {
			formatted << hex.str() << "     " << ascii.str() << "\n";
			hex.str(""); hex.clear();
			ascii.str(""); ascii.clear();
			hex << std::hex << std::setfill('0');
		}
	}

	return formatted.str();
}

}} // util, ember