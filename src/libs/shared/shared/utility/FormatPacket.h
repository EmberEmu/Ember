/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <gsl/narrow>
#include <iomanip>
#include <sstream>
#include <span>
#include <cstddef>
#include <cctype>

namespace ember::util {

inline std::string format_packet(const auto* packet, std::size_t size, unsigned int columns = 16) {
	auto data = reinterpret_cast<const unsigned char*>(packet);
	const auto rows = size / columns + (size % columns? 1 : 0); // have faith, compiler will optimise this

	std::stringstream buffer;
	std::size_t offset = 0u;

	for(std::size_t i = 0; i != rows; ++i) {
		buffer << std::hex << std::setw(4) << std::setfill('0') << i * columns << "   ";

		for(std::size_t j = 0; j < columns; ++j) {
			buffer << std::setfill('0');

			if(j + offset < size) {
				buffer << std::setw(2) << gsl::narrow_cast<unsigned>(data[j + offset]) << " ";
			} else {
				buffer << "   ";
			}
		}

		buffer << "     ";

		for(std::size_t j = 0; j < columns; ++j) {
			if(j + offset < size) {
				buffer << gsl::narrow_cast<char>(std::isprint(data[j + offset])? data[j + offset] : '.');
			} else {
				buffer << " ";
			}
		}

		offset += columns;

		if(i != rows - 1) {
			buffer << "\n";
		}
	}

	return buffer.str();
}

template<typename T>
inline std::string format_packet(std::span<const T> packet, unsigned int columns = 16) {
	return format_packet(packet.data(), packet.size_bytes(), columns);
}

} // util, ember