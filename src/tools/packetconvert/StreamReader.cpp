/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamReader.h"
#include <boost/endian/conversion.hpp>
#include <boost/container/small_vector.hpp>
#include <thread>
#include <iostream>

namespace ember {

StreamReader::StreamReader(std::ifstream& in, std::uintmax_t size, bool stream,
                           bool skip, std::chrono::milliseconds interval)
	: in_(in)
	, skip_(skip)
	, stream_(stream)
	, interval_(interval)
	, stream_size_(size) {
	if(!in) {
		throw std::runtime_error("Packet dump stream error");
	}
}

void StreamReader::add_sink(std::unique_ptr<Sink> sink) {
	sinks_.emplace_back(std::move(sink));
}

void StreamReader::process() {
	auto state = ReadState::size;
	std::optional<fblog::Type> type;
	boost::container::small_vector<std::uint8_t, 256> buffer;

	while(stream_ || stream_size_ != in_.tellg()) {
		if(state == ReadState::size) {
			auto size = try_read<std::uint32_t>(in_);

			if(!size) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*size);
			buffer.resize(*size, boost::container::default_init);
			state = ReadState::type;
		}

		if(state == ReadState::type) {
			type = try_read<fblog::Type>(in_);

			if(!type) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*type);
			state = ReadState::body;
		}

		if(state == ReadState::body) {
			auto ret = try_read(in_, buffer);

			if(!ret) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			if(!skip_) {
				handle_buffer(*type, buffer);
			}

			state = ReadState::size;
		}
	}
}

template<typename T>
std::optional<T> StreamReader::try_read(std::ifstream& file) {
	static_assert(std::is_trivially_copyable_v<T>, "Cannot safely read this type");

	T val;

	const auto pos = file.tellg();
	file.read(reinterpret_cast<char*>(&val), sizeof(val));

	if(file) {
		return val;
	} else {
		file.clear();
		file.seekg(pos);
		return std::nullopt;
	}
}

bool StreamReader::try_read(std::ifstream& file, std::span<std::uint8_t> buffer) {
	const auto pos = file.tellg();

	file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	if(file) {
		return true;
	} else {
		file.clear();
		file.seekg(pos);
		return false;
	}
}

template<typename flatbuffer_type>
void StreamReader::dispatch(std::span<const std::uint8_t> buff) {
	const auto message = flatbuffers::GetRoot<flatbuffer_type>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!message->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	for(auto& sink : sinks_) {
		sink->handle(*message);
	}
}

void StreamReader::handle_buffer(const fblog::Type type, std::span<const std::uint8_t> buff) {
	using enum fblog::Type;

	switch(type) {
		case header:
			dispatch<fblog::Header>(buff);
			break;
		case message:
			dispatch<fblog::Message>(buff);
			break;
		case footer:
			dispatch<fblog::Footer>(buff);
			break;
		default:
			throw std::runtime_error("Unknown message type");
	}
}

} // ember