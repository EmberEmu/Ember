/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <iomanip>
#include <sstream>
#include <cstddef>
#include <cctype>
#include <cmath>

namespace ember::util {

inline std::string format_packet(const unsigned char* packet, std::size_t size,
                                 unsigned int columns = 16) {
	auto rows = static_cast<std::size_t>(std::ceil(size / static_cast<double>(columns)));
	std::stringstream buffer;
	auto offset = 0;

	for(std::size_t i = 0; i != rows; ++i) {
		buffer << std::hex << std::setw(4) << std::setfill('0') << i * columns << "   ";

		for(std::size_t j = 0; j < columns; ++j) {
			buffer << std::hex << std::setfill('0');

			if(j + offset < size) {
				buffer << std::setw(2) << static_cast<unsigned>(packet[j + offset]) << " ";
			} else {
				buffer << "   ";
			}
		}

		buffer << "     ";

		for(std::size_t j = 0; j < columns; ++j) {
			if(j + offset < size) {
				buffer << static_cast<char>(std::isprint(packet[j + offset])? packet[j + offset] : '.');
			} else {
				buffer << " ";
			}
		}

		offset += columns;
		buffer << "\n";
	}

	return buffer.str();
}

} // util, ember