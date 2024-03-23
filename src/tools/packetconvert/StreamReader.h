/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sink.h"
#include "PacketLog_generated.h"
#include <chrono>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <cstdint>

namespace ember {

class StreamReader final {
	enum class ReadState {
		SIZE, TYPE, BODY
	};

	std::ifstream& in_;
	bool skip_;
	const bool stream_;
	const std::chrono::seconds interval_;
	std::streampos stream_size_;
	std::vector<std::unique_ptr<Sink>> sinks_;

	void handle_buffer(const fblog::Type type, std::span<const std::uint8_t> buff);
	void handle_message(std::span<const std::uint8_t> buff);
	void handle_header(std::span<const std::uint8_t> buff);
	bool try_read(std::ifstream& file, std::vector<std::uint8_t>& buffer);
	template<typename T> std::optional<T> try_read(std::ifstream& file);

public:
	StreamReader(std::ifstream& in, std::uintmax_t size, bool stream, bool skip = false,
	             std::chrono::seconds interval = std::chrono::seconds(2));

	void process();
	void add_sink(std::unique_ptr<Sink> sink);
};

} // ember